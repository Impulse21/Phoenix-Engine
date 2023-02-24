#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/SceneLoader.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Renderer/VisibilityBufferPasses.h>
#include <PhxEngine/Renderer/DeferredLightingPass.h>
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Renderer/GBuffer.h>
#include <PhxEngine/Engine/CameraControllers.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

struct VisibilityIndirectDraw
{
    uint32_t InstanceIdx;
    IndirectDrawArgsIndexedInstanced Draw;
};

constexpr uint32_t kMaxDrawPackets = 10;

// Add one so there is space for root constant
// 5 + 3 == 8 Extra room for counter and any root constants..
constexpr size_t kIndirectDrawStructStride = sizeof(VisibilityIndirectDraw);

// The following value defines the maximum amount of indirect draw calls that will be
// drawn at once. This value depends on the number of submeshes or individual objects
// in the scene. Changing a scene will require to change this value accordingly.
constexpr size_t kMaxDrawIndirect = kMaxDrawPackets;

// The following values point to the position in the indirect draw buffer that holds the
// number of draw calls to draw after triangle filtering and batch compaction.
// This value number is stored in the last position of the indirect draw buffer.
// So it depends on MAX_DRAWS_INDIRECT
constexpr size_t kDrawCounterSlotPos = (kMaxDrawIndirect - 1) * kIndirectDrawStructStride;
constexpr size_t kDrawCounterSlotOffsetInBytes = kDrawCounterSlotPos * sizeof(uint32_t);

struct RenderableScene
{
    RHI::BufferHandle GlobalIndexBuffer;
    RHI::BufferHandle ObjectInstanceBuffer;
    RHI::BufferHandle GeometryBufferHandle;
    RHI::BufferHandle DrawPacketBufferHandle;
    RHI::BufferHandle IndirectDrawArgBufferAll;
    RHI::BufferHandle FrameConstantBuffer;

    void PrepareRenderData(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene)
    {
        if (this->FrameConstantBuffer.IsValid())
        {
            gfxDevice->DeleteBuffer(this->FrameConstantBuffer);
        }

        this->FrameConstantBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
            .Usage = RHI::Usage::Default,
            .Binding = RHI::BindingFlags::ConstantBuffer,
            .InitialState = RHI::ResourceStates::ConstantBuffer,
            .SizeInBytes = sizeof(Shader::Frame),
            .DebugName = "Frame Constant Buffer",
            });

        //Fill Global Vertex and index Buffers;
        this->MergeMeshes(gfxDevice, commandList, scene);
        auto geometryUploadBuffer = this->BuildGeometryBuffer(gfxDevice, scene);
        auto objectUploadBuffer = this->BuildObjectInstanceBuffer(gfxDevice, scene);
        auto drawPacketUploader = this->CreateDrawPacketBuffer(gfxDevice, scene);
        auto indirectArgDrawAllBuffer = this->BuildIndirectDrawArgBufferAll(gfxDevice, scene, drawPacketUploader);

        // Upload data
        RHI::GpuBarrier preCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->IndirectDrawArgBufferAll, RHI::ResourceStates::IndirectArgument, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(this->FrameConstantBuffer, RHI::ResourceStates::ConstantBuffer, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(this->DrawPacketBufferHandle, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(this->GeometryBufferHandle, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(this->ObjectInstanceBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

        Shader::Frame_NEW frame = {};
        frame.SceneData = {};
        frame.SceneData.ObjectBufferIdx = gfxDevice->GetDescriptorIndex(this->ObjectInstanceBuffer, SubresouceType::SRV);
        frame.SceneData.GeometryBufferIdx = gfxDevice->GetDescriptorIndex(this->GeometryBufferHandle, SubresouceType::SRV);
        frame.SceneData.GlobalVertexBufferIdx = RHI::cInvalidDescriptorIndex;
        frame.SceneData.GlobalIndexxBufferIdx = gfxDevice->GetDescriptorIndex(this->GlobalIndexBuffer, SubresouceType::SRV);
        frame.SceneData.DrawPacketBufferIdx = gfxDevice->GetDescriptorIndex(this->DrawPacketBufferHandle, SubresouceType::SRV);
        frame.SceneData.MaterialBufferIdx = RHI::cInvalidDescriptorIndex;
        frame.SceneData.MeshletBufferIndex = RHI::cInvalidDescriptorIndex;

        commandList->WriteBuffer(this->FrameConstantBuffer, frame);

        commandList->CopyBuffer(
            this->DrawPacketBufferHandle,
            0,
            drawPacketUploader.UploadBuffer,
            0,
            gfxDevice->GetBufferDesc(this->DrawPacketBufferHandle).SizeInBytes);

        commandList->CopyBuffer(
            this->IndirectDrawArgBufferAll,
            0,
            indirectArgDrawAllBuffer.UploadBuffer,
            0,
            gfxDevice->GetBufferDesc(this->IndirectDrawArgBufferAll).SizeInBytes);

        commandList->CopyBuffer(
            this->GeometryBufferHandle,
            0,
            geometryUploadBuffer.UploadBuffer,
            0,
            gfxDevice->GetBufferDesc(this->GeometryBufferHandle).SizeInBytes);

        commandList->CopyBuffer(
            this->ObjectInstanceBuffer,
            0,
            objectUploadBuffer.UploadBuffer,
            0,
            gfxDevice->GetBufferDesc(this->ObjectInstanceBuffer).SizeInBytes);

        RHI::GpuBarrier postCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->IndirectDrawArgBufferAll, RHI::ResourceStates::CopyDest, RHI::ResourceStates::IndirectArgument),
            RHI::GpuBarrier::CreateBuffer(this->FrameConstantBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ConstantBuffer),
            RHI::GpuBarrier::CreateBuffer(this->DrawPacketBufferHandle, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(this->GeometryBufferHandle, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(this->ObjectInstanceBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));

        geometryUploadBuffer.Free();
        objectUploadBuffer.Free();
        indirectArgDrawAllBuffer.Free();
    }

    void MergeMeshes(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene)
    {
        uint64_t indexCount = 0;
        uint64_t indexSizeInBytes = 0;

        auto view = scene.GetAllEntitiesWith<Scene::MeshComponent>();
        for (auto e : view)
        {
            auto& meshComp = view.get<Scene::MeshComponent>(e);

            meshComp.GlobalIndexBufferOffset = indexCount;
            indexCount += meshComp.TotalIndices;
            indexSizeInBytes += meshComp.GetIndexBufferSizeInBytes();
        }

        if (this->GlobalIndexBuffer.IsValid())
        {
            gfxDevice->DeleteBuffer(this->GlobalIndexBuffer);
        }

        RHI::BufferDesc indexBufferDesc = {};
        indexBufferDesc.SizeInBytes = indexSizeInBytes;
        indexBufferDesc.StrideInBytes = sizeof(uint32_t);
        indexBufferDesc.DebugName = "Index Buffer";
        indexBufferDesc.Binding = RHI::BindingFlags::IndexBuffer;
        this->GlobalIndexBuffer = RHI::IGraphicsDevice::GPtr->CreateIndexBuffer(indexBufferDesc);

        // Upload data
        {
            RHI::GpuBarrier preCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(this->GlobalIndexBuffer, gfxDevice->GetBufferDesc(this->GlobalIndexBuffer).InitialState, RHI::ResourceStates::CopyDest),
            };

            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));
        }

        for (auto e : view)
        {
            auto& meshComp = view.get<Scene::MeshComponent>(e);;

            BufferDesc vertexBufferDesc = gfxDevice->GetBufferDesc(meshComp.VertexGpuBuffer);
            BufferDesc indexBufferDesc = gfxDevice->GetBufferDesc(meshComp.IndexGpuBuffer);
            RHI::GpuBarrier preCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(meshComp.IndexGpuBuffer, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopySource),
            };

            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

            commandList->CopyBuffer(
                this->GlobalIndexBuffer,
                meshComp.GlobalIndexBufferOffset * indexBufferDesc.StrideInBytes,
                meshComp.IndexGpuBuffer,
                0,
                indexBufferDesc.SizeInBytes);

            // Upload data
            RHI::GpuBarrier postCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(meshComp.IndexGpuBuffer, RHI::ResourceStates::CopySource, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource),
            };

            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
        }


        // Upload data
        RHI::GpuBarrier postCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->GlobalIndexBuffer, RHI::ResourceStates::CopyDest, gfxDevice->GetBufferDesc(this->GlobalIndexBuffer).InitialState),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
    }

    ResourceUpload BuildGeometryBuffer(IGraphicsDevice* gfxDevice, Scene::Scene& scene)
    {
        auto view = scene.GetAllEntitiesWith<Scene::MeshComponent>();
        uint32_t numGeometryEntires = 0;
        for (auto entity : view)
        {
            auto& meshComponent = view.get<Scene::MeshComponent>(entity);
            numGeometryEntires += meshComponent.Surfaces.size();

            meshComponent.RenderBucketMask = Scene::MeshComponent::RenderType::RenderType_Opaque;
            for (auto& surface : meshComponent.Surfaces)
            {
                const auto& material = scene.GetRegistry().get<Scene::MaterialComponent>(surface.Material);
                if (material.BlendMode == Renderer::BlendMode::Alpha)
                {
                    meshComponent.RenderBucketMask = Scene::MeshComponent::RenderType::RenderType_Transparent;
                    break;
                }
            }
        }

        if (this->GeometryBufferHandle.IsValid())
        {
            gfxDevice->DeleteBuffer(this->GeometryBufferHandle);
        }

        this->GeometryBufferHandle = gfxDevice->CreateBuffer({
                .MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured,
                .Binding = RHI::BindingFlags::ShaderResource,
                .InitialState = ResourceStates::ShaderResource,
                .StrideInBytes = sizeof(Shader::Geometry),
                .SizeInBytes = sizeof(Shader::Geometry) * numGeometryEntires,
                .CreateBindless = true,
                .DebugName = "Geometry Data",
            });

        // -- Update data --
        ResourceUpload geometryUpload = Renderer::CreateResourceUpload(numGeometryEntires * sizeof(Shader::Geometry));
        Shader::Geometry* geometryBufferMappedData = (Shader::Geometry*)geometryUpload.Data;
        uint32_t currGeoIndex = 0;
        for (auto entity : view)
        {
            auto& mesh = view.get<Scene::MeshComponent>(entity);
            mesh.GlobalGeometryBufferIndex = currGeoIndex;
            for (int i = 0; i < mesh.Surfaces.size(); i++)
            {
                Scene::MeshComponent::SurfaceDesc& surfaceDesc = mesh.Surfaces[i];
                surfaceDesc.GlobalGeometryBufferIndex = currGeoIndex++;

                Shader::Geometry* geometryShaderData = geometryBufferMappedData + surfaceDesc.GlobalGeometryBufferIndex;
                auto& material = scene.GetRegistry().get<Scene::MaterialComponent>(surfaceDesc.Material);
                geometryShaderData->MaterialIndex = material.GlobalBufferIndex;
                geometryShaderData->NumIndices = surfaceDesc.NumIndices;
                geometryShaderData->NumVertices = surfaceDesc.NumVertices;
                geometryShaderData->MeshletOffset = mesh.MeshletCount;
                geometryShaderData->MeshletCount = ((surfaceDesc.NumIndices / 3) + 1) / Shader::MESHLET_TRIANGLE_COUNT;
                mesh.MeshletCount += geometryShaderData->MeshletCount;
                geometryShaderData->IndexOffset = surfaceDesc.IndexOffsetInMesh;
                geometryShaderData->VertexBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(mesh.VertexGpuBuffer, RHI::SubresouceType::SRV);
                geometryShaderData->PositionOffset = mesh.GetVertexAttribute(Scene::MeshComponent::VertexAttribute::Position).ByteOffset;
                geometryShaderData->TexCoordOffset = mesh.GetVertexAttribute(Scene::MeshComponent::VertexAttribute::TexCoord).ByteOffset;
                geometryShaderData->NormalOffset = mesh.GetVertexAttribute(Scene::MeshComponent::VertexAttribute::Normal).ByteOffset;
                geometryShaderData->TangentOffset = mesh.GetVertexAttribute(Scene::MeshComponent::VertexAttribute::Tangent).ByteOffset;
            }
        }

        return geometryUpload;
    }

    ResourceUpload BuildObjectInstanceBuffer(IGraphicsDevice* gfxDevice, Scene::Scene& scene)
    {
        auto meshInstanceView = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent>();
        uint32_t numObjects = meshInstanceView.size();
        if (this->ObjectInstanceBuffer.IsValid())
        {
            gfxDevice->DeleteBuffer(ObjectInstanceBuffer);
        }

        this->ObjectInstanceBuffer = gfxDevice->CreateBuffer({
                .MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured,
                .Binding = RHI::BindingFlags::ShaderResource,
                .InitialState = ResourceStates::ShaderResource,
                .StrideInBytes = sizeof(Shader::MeshInstance),
                .SizeInBytes = sizeof(Shader::MeshInstance) * numObjects,
                .CreateBindless = true,
                .DebugName = "Object Data",
            });

        auto view = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::TransformComponent>();
        for (auto e : view)
        {
            auto [meshInstanceComponent, transformComponent] = view.get<Scene::MeshInstanceComponent, Scene::TransformComponent>(e);
            meshInstanceComponent.WorldMatrix = transformComponent.WorldMatrix;
        }
        ResourceUpload objectUploadBuffer = Renderer::CreateResourceUpload(numObjects * sizeof(Shader::MeshInstance));
        Shader::MeshInstance* pUploadBufferData = (Shader::MeshInstance*)objectUploadBuffer.Data;
        uint32_t currInstanceIndex = 0;
        for (auto e : view)
        {
            auto [meshInstanceComponent, transformComponent] = view.get<Scene::MeshInstanceComponent, Scene::TransformComponent>(e);
            Shader::MeshInstance* shaderMeshInstance = pUploadBufferData + currInstanceIndex;

            auto& mesh = scene.GetRegistry().get<Scene::MeshComponent>(meshInstanceComponent.Mesh);
            shaderMeshInstance->GeometryOffset = mesh.GlobalGeometryBufferIndex;
            shaderMeshInstance->GeometryCount = (uint)mesh.Surfaces.size();
            shaderMeshInstance->WorldMatrix = transformComponent.WorldMatrix;
            shaderMeshInstance->MeshletOffset = mesh.MeshletCount;
            // this->m_numMeshlets += mesh.MeshletCount;

            meshInstanceComponent.GlobalBufferIndex = currInstanceIndex++;
        }

        return objectUploadBuffer;
    }

    ResourceUpload CreateDrawPacketBuffer(IGraphicsDevice* gfxDevice, Scene::Scene& scene)
    {
        if (this->DrawPacketBufferHandle.IsValid())
        {
            gfxDevice->DeleteBuffer(this->DrawPacketBufferHandle);
        }

        this->DrawPacketBufferHandle = gfxDevice->CreateBuffer({
            .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::Bindless,
            .Binding = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
            .InitialState = ResourceStates::ShaderResource,
            .StrideInBytes = sizeof(Shader::DrawPacket),
            .SizeInBytes = sizeof(Shader::DrawPacket) * kMaxDrawPackets,
            .AllowUnorderedAccess = true });

        return Renderer::CreateResourceUpload(gfxDevice->GetBufferDesc(this->DrawPacketBufferHandle).SizeInBytes);
    }

    ResourceUpload BuildIndirectDrawArgBufferAll(IGraphicsDevice* gfxDevice, Scene::Scene& scene, ResourceUpload& drawPacketUploader)
    {
        if (this->IndirectDrawArgBufferAll.IsValid())
        {
            gfxDevice->DeleteBuffer(this->IndirectDrawArgBufferAll);
        }
        
        const size_t maxDrawCount = kMaxDrawIndirect;
        const size_t sizeInBytes = kMaxDrawIndirect * kIndirectDrawStructStride + sizeof(uint32_t);
        this->IndirectDrawArgBufferAll = gfxDevice->CreateBuffer({
            .MiscFlags = BufferMiscFlags::Structured,
            .Binding = BindingFlags::UnorderedAccess,
            .InitialState = ResourceStates::IndirectArgument,
            .StrideInBytes = kIndirectDrawStructStride,
            .SizeInBytes = sizeInBytes,
            .AllowUnorderedAccess = true });

        // Number of Draw batches
        ResourceUpload indirectDrawUpload = Renderer::CreateResourceUpload(sizeInBytes);
        auto objectInstanceView = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent>();
        uint32_t drawIndex = 0;

        for (auto e : objectInstanceView)
        {
            auto& objectInstance = objectInstanceView.get<Scene::MeshInstanceComponent>(e);

            auto& meshComponent = scene.GetRegistry().get<Scene::MeshComponent>(objectInstance.Mesh);
            if (meshComponent.RenderBucketMask != meshComponent.RenderType_Opaque)
            {
                continue;
            }
            for (auto& s : meshComponent.Surfaces)
            {
                auto packet = (Shader::DrawPacket*)drawPacketUploader.Data + drawIndex;
                packet->GeometryIdx = s.GlobalGeometryBufferIndex;
                packet->InstanceIdx = objectInstance.GlobalBufferIndex;

                VisibilityIndirectDraw args = {};
                args.InstanceIdx = drawIndex;
                RHI::IndirectDrawArgsIndexedInstanced& drawArg = args.Draw;
                // drawArg.StartInstance = drawIndex; // Draw ID
                drawArg.StartInstance = 0; 
                drawArg.InstanceCount = 1;
                drawArg.IndexCount = s.NumIndices;
                drawArg.StartIndex = meshComponent.GlobalIndexBufferOffset + s.IndexOffsetInMesh;
                drawArg.VertexOffset = 0;

                indirectDrawUpload.SetData(&args, kIndirectDrawStructStride, kIndirectDrawStructStride);
                drawIndex++;
            }
        }

        uint32_t* counter = reinterpret_cast<uint32_t*>(indirectDrawUpload.Data) + (sizeInBytes / sizeof(uint32_t) - 1);
        *counter = drawIndex;

        std::array<uint32_t, sizeInBytes / 4> testing;

        std::memcpy(testing.data(), indirectDrawUpload.Data, sizeInBytes);

        return indirectDrawUpload;
    }
};

struct SceneVisibilityRenderer3D
{
    SceneVisibilityRenderer3D(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<PhxEngine::Renderer::CommonPasses> commonPasses)
        : m_gfxDevice(gfxDevice)
        , m_commonPasses(commonPasses)
    {
    }

    void WindowResize(DirectX::XMFLOAT2 const& canvasSize)
    {
        this->m_canvasSize = canvasSize;

        if (this->m_depthBuffer.IsValid())
        {
            this->m_gfxDevice->DeleteTexture(this->m_depthBuffer);
        }

        if (this->m_visibilityBuffer.IsValid())
        {
            this->m_gfxDevice->DeleteTexture(this->m_visibilityBuffer);
        }

        if (this->m_visFillRenderPass.IsValid())
        {
            this->m_gfxDevice->DeleteRenderPass(this->m_visFillRenderPass);
        }

        RHI::TextureDesc desc = {};
        desc.Width = std::max(canvasSize.x, 1.0f);
        desc.Height = std::max(canvasSize.y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.IsBindless = false;

        desc.Format = RHI::RHIFormat::D32;
        desc.IsTypeless = true;
        desc.DebugName = "Depth Buffer";
        desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
        desc.InitialState = RHI::ResourceStates::DepthRead;
        this->m_depthBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.OptmizedClearValue.Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
        desc.InitialState = RHI::ResourceStates::ShaderResource;
        desc.IsBindless = true;
        desc.IsTypeless = true;
        desc.Format = RHI::RHIFormat::RGBA8_UNORM;
        desc.DebugName = "Visibility Buffer";
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::RenderTarget;

        this->m_visibilityBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);
        this->m_visFillRenderPass = IGraphicsDevice::GPtr->CreateRenderPass(
            {
                .Attachments =
                {
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->m_visibilityBuffer,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .Type = RenderPassAttachment::Type::DepthStencil,
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->m_depthBuffer,
                        .InitialLayout = RHI::ResourceStates::DepthRead,
                        .SubpassLayout = RHI::ResourceStates::DepthWrite,
                        .FinalLayout = RHI::ResourceStates::DepthRead
                    },
                }
            });
    }

    void Initialize(Graphics::ShaderFactory& factory, DirectX::XMFLOAT2 const& canvasSize)
    {
        this->m_cullPassCS = factory.CreateShader(
            "PhxEngine/CullPass.hlsl",
            {
                .Stage = RHI::ShaderStage::Compute,
                .DebugName = "CullPassCS",
            });

        this->m_visFillPassMS = factory.CreateShader(
            "PhxEngine/VisibilityBufferFillPassMS.hlsl",
            {
                .Stage = RHI::ShaderStage::Mesh,
                .DebugName = "VisibilityBufferFillPassMS",
            });

        this->m_visFillPassVS = factory.CreateShader(
            "PhxEngine/VisibilityBufferFillPassVS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "VisibilityBufferFillPassVS",
            });

        this->m_visFillPassPS = factory.CreateShader(
            "PhxEngine/VisibilityBufferFillPassPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "VisibilityBufferFillPassPS",
            });

        // TODO: Deferred Lighting

        this->WindowResize(canvasSize);
    }

    void Render(RenderableScene const& renderableScene, Scene::CameraComponent mainCamera, RHI::Viewport& viewport)
    {
        // Do Render
        this->ConstructPipelines();

        Shader::Camera cameraData = {};
        cameraData.ViewProjection = mainCamera.ViewProjection;
        cameraData.ViewProjectionInv = mainCamera.ViewProjectionInv;
        cameraData.ProjInv = mainCamera.ProjectionInv;
        cameraData.ViewInv = mainCamera.ViewInv;

        RHI::ICommandList* commandList = this->m_gfxDevice->BeginCommandRecording();
        {
#if false
            auto _ = commandList->BeginScopedMarker("Visibility Cull Pass");

            // TODO: Change to match same syntax as GFX pipeline
            commandList->SetComputeState(this->m_pipelineCullPipeline);

            commandList->BindConstantBuffer(1, input.FrameBuffer);
            commandList->BindDynamicConstantBuffer(2, *input.CameraData);
#endif
        }

        {
            auto _ = commandList->BeginScopedMarker("Visibility Buffer Fill Pass");
            commandList->BeginRenderPass(this->m_visFillRenderPass);
            commandList->SetViewports(&viewport, 1);
            RHI::Rect rec(LONG_MAX, LONG_MAX);
            commandList->SetScissors(&rec, 1);

            if (this->EnableMeshPipeline)
            {
#if false
                commandList->SetMeshPipeline(this->m_pipelineVisFillMesh);
                auto view = this->m_scene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::AABBComponent>();
                for (auto e : view)
                {
                    auto [instanceComponent, aabbComp] = view.get<Scene::MeshInstanceComponent, Scene::AABBComponent>(e);

                    auto& meshComponent = this->m_scene.GetRegistry().get<Scene::MeshComponent>(instanceComponent.Mesh);

                    Shader::MeshletPushConstants pushConstant = {};
                    pushConstant.WorldMatrix = instanceComponent.WorldMatrix;
                    pushConstant.VerticesBufferIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.VertexGpuBuffer, RHI::SubresouceType::SRV);
                    pushConstant.MeshletsBufferIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.Meshlets, RHI::SubresouceType::SRV);
                    pushConstant.PrimitiveIndicesIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.PrimitiveIndices, RHI::SubresouceType::SRV);
                    pushConstant.UniqueVertexIBIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.UniqueVertexIndices, RHI::SubresouceType::SRV);
                    pushConstant.GeometryIdx = meshComponent.Surfaces.front().GlobalGeometryBufferIndex;


                    for (auto& subset : meshComponent.MeshletSubsets)
                    {
                        pushConstant.SubsetOffset = subset.Offset;
                        commandList->BindPushConstant(0, pushConstant);
                        commandList->DispatchMesh(subset.Count, 1, 1);
                    }
                }
#endif 
            }
            else
            {
                commandList->SetGraphicsPipeline(this->m_pipelineVisFillGfx);

                commandList->BindIndexBuffer(renderableScene.GlobalIndexBuffer);
                commandList->BindConstantBuffer(1, renderableScene.FrameConstantBuffer);
                commandList->BindDynamicConstantBuffer(2, cameraData);

                commandList->ExecuteIndirect(
                    this->m_indirectDrawSignature,
                    renderableScene.IndirectDrawArgBufferAll,
                    0,
                    renderableScene.IndirectDrawArgBufferAll,
                    kMaxDrawIndirect * kIndirectDrawStructStride,
                    kMaxDrawIndirect);
            }
            commandList->EndRenderPass();
        }

        this->m_commonPasses->BlitTexture(commandList, this->m_visibilityBuffer, commandList->GetRenderPassBackBuffer(), this->m_canvasSize);
        commandList->Close();
        this->m_gfxDevice->ExecuteCommandLists({ commandList });
    }

    void ConstructPipelines()
    {
        if (!this->m_pipelineVisFillGfx.IsValid())
        {
            this->m_pipelineVisFillGfx = this->m_gfxDevice->CreateGraphicsPipeline(
                {
                    .VertexShader = this->m_visFillPassVS,
                    .PixelShader = this->m_visFillPassPS,
                    .RtvFormats = { this->m_gfxDevice->GetTextureDesc(this->m_visibilityBuffer).Format },
                    .DsvFormat = { this->m_gfxDevice->GetTextureDesc(this->m_depthBuffer).Format }
                });

            // Construct Command Signature
            this->m_indirectDrawSignature = this->m_gfxDevice->CreateCommandSignature<VisibilityIndirectDraw>({
                    .ArgDesc = 
                        {
                            {.Type = IndirectArgumentType::Constant, .Constant = { .RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
                            {.Type = IndirectArgumentType::DrawIndex }
                        },
                    .PipelineType = PipelineType::Gfx,
                    .GfxHandle = this->m_pipelineVisFillGfx
                });
        }

        if (!this->m_pipelineVisFillMesh.IsValid())
        {
#if false
            this->m_pipelineVisFillMesh = this->m_gfxDevice->CreateMeshPipeline(
                {
                    .MeshShader = this->m_visFillPassMS,
                    .PixelShader = this->m_visFillPassPS,
                    .RtvFormats = { this->m_gfxDevice->GetTextureDesc(this->m_visibilityBuffer).Format },
                    .DsvFormat = { this->m_gfxDevice->GetTextureDesc(this->m_depthBuffer).Format }
                });
#endif
        }

        if (!this->m_pipelineCullPipeline.IsValid())
        {
#if false
            this->m_pipelineCullPipeline = this->m_gfxDevice->CreateComputePipeline(
                {
                    .ComputeShader = this->m_cullPassCS,
                });
#endif
        }
    }

    bool EnableMeshPipeline = false;

private:
    DirectX::XMFLOAT2 m_canvasSize;
    RHI::IGraphicsDevice* m_gfxDevice;
    std::shared_ptr<PhxEngine::Renderer::CommonPasses> m_commonPasses;

    RHI::RenderPassHandle m_visFillRenderPass;
    RHI::TextureHandle m_visibilityBuffer;
    RHI::TextureHandle m_depthBuffer;

    RHI::RenderPassHandle m_lightingRenderPass;
    RHI::TextureHandle m_colourBuffer;

    RHI::ShaderHandle m_cullPassCS;

    RHI::ShaderHandle m_visFillPassVS;
    RHI::ShaderHandle m_visFillPassMS;
    RHI::ShaderHandle m_visFillPassPS;

    RHI::ShaderHandle m_visDeferredLightPassCS;

    RHI::GraphicsPipelineHandle m_pipelineVisFillGfx;
    RHI::CommandSignatureHandle m_indirectDrawSignature;
    RHI::MeshPipelineHandle m_pipelineVisFillMesh;

    RHI::ComputePipelineHandle m_pipelineCullPipeline;
    RHI::ComputePipelineHandle m_pipelineVisDeferredPass;



};


class VisibilityRenderering : public ApplicationBase
{
private:

public:
    VisibilityRenderering(IPhxEngineRoot* root)
        : ApplicationBase(root)
    {
    }

    bool Initialize()
    {
        // Load Data in seperate thread?
        // Create File Sustem
        std::shared_ptr<Core::IFileSystem> nativeFS = Core::CreateNativeFileSystem();

        std::filesystem::path appShadersRoot = Core::Platform::GetExcecutableDir() / "Shaders/PhxEngine/dxil";
        std::shared_ptr<Core::IRootFileSystem> rootFilePath = Core::CreateRootFileSystem();
        rootFilePath->Mount("/Shaders/PhxEngine", appShadersRoot);

        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath, "/Shaders");
        this->m_commonPasses = std::make_shared<Renderer::CommonPasses>(this->GetGfxDevice(), *this->m_shaderFactory);
        this->m_sceneRenderer = std::make_unique<SceneVisibilityRenderer3D>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_sceneRenderer->Initialize(*this->m_shaderFactory, this->GetRoot()->GetCanvasSize());
        this->m_toneMappingPass = std::make_unique<Renderer::ToneMappingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_toneMappingPass->Initialize(*this->m_shaderFactory);

        this->m_textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/TestScenes/VisibilityBufferScene.gltf";
        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_mainCamera.FoV = DirectX::XMConvertToRadians(60);

        Scene::TransformComponent t = {};
        t.LocalTranslation = { -5.0f, 2.0f, 0.0f };
        t.RotateRollPitchYaw({ 0.0f, DirectX::XMConvertToRadians(90), 0.0f });
        t.UpdateTransform();

        this->m_mainCamera.TransformCamera(t);
        this->m_mainCamera.UpdateCamera();

        return true;
    }

    bool LoadScene(std::shared_ptr< Core::IFileSystem> fileSystem, std::filesystem::path sceneFilename) override
    {
        std::unique_ptr<Scene::ISceneLoader> sceneLoader = PhxEngine::Scene::CreateGltfSceneLoader();
        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        bool retVal = sceneLoader->LoadScene(
            fileSystem,
            this->m_textureCache,
            sceneFilename,
            commandList,
            this->m_scene);

        if (retVal)
        {
            Renderer::ResourceUpload indexUpload;
            Renderer::ResourceUpload vertexUpload;
            this->m_scene.ConstructRenderData(commandList, indexUpload, vertexUpload);
            /*
            auto view = this->m_scene.GetAllEntitiesWith<Scene::MeshComponent>();
            for (auto e : view)
            {
                view.get<Scene::MeshComponent>(e).ComputeMeshlets(this->GetGfxDevice(), commandList);
            }
            */
            indexUpload.Free();
            vertexUpload.Free();

            this->m_renderableScene.PrepareRenderData(this->GetGfxDevice(), commandList, this->m_scene);
        }

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteMeshPipeline(this->m_pipeline);
        this->m_pipeline = {};
        this->m_sceneRenderer->WindowResize(this->GetRoot()->GetCanvasSize());
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->m_rotation += deltaTime.GetSeconds() * 1.1f;
        this->GetRoot()->SetInformativeWindowTitle("Example: Deferred Rendering", {});
        this->m_cameraController.OnUpdate(this->GetRoot()->GetWindow(), deltaTime, this->m_mainCamera);
    }

    void RenderScene() override
    {
		this->m_mainCamera.Width = this->GetRoot()->GetCanvasSize().x;
		this->m_mainCamera.Height = this->GetRoot()->GetCanvasSize().y;
		this->m_mainCamera.UpdateCamera();

        auto canvas = this->GetRoot()->GetCanvasSize();
        RHI::Viewport v(canvas.x, canvas.y);
        this->m_sceneRenderer->Render(this->m_renderableScene, this->m_mainCamera, v);
    }

private:
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::MeshPipelineHandle m_pipeline;
    RHI::ShaderHandle m_meshShader;
    RHI::ShaderHandle m_pixelShader;

    Scene::Scene m_scene;
    RenderableScene m_renderableScene;

    Scene::CameraComponent m_mainCamera;
    PhxEngine::FirstPersonCameraController m_cameraController;

    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
    std::unique_ptr<SceneVisibilityRenderer3D> m_sceneRenderer;
    std::unique_ptr<Renderer::ToneMappingPass> m_toneMappingPass;

    RHI::TextureHandle m_depthTex;

    float m_rotation = 0.0f;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: Visibility Buffer";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    root->Initialize(params);

    {
        VisibilityRenderering example(root.get());
        if (example.Initialize())
        {
            root->AddPassToBack(&example);
            root->Run();
            root->RemovePass(&example);
        }
    }

    root->Finalizing();
    root.reset();
    RHI::ReportLiveObjects();

    return 0;
}
