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
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;

struct GBufferRenderTargets
{
    RHI::TextureHandle DepthTex;
    RHI::TextureHandle AlbedoTex;
    RHI::TextureHandle NormalTex;
    RHI::TextureHandle SurfaceTex;
    RHI::TextureHandle SpecularTex;

    RHI::RenderPassHandle RenderPass;
    DirectX::XMFLOAT2 CanvasSize;


    // Extra
    RHI::TextureHandle FinalColourBuffer;

    void Initialize(
        RHI::IGraphicsDevice* gfxDevice,
        DirectX::XMFLOAT2 const& size)
    {
        CanvasSize = size;

        // -- Depth ---
        RHI::TextureDesc desc = {};
        desc.Width = std::max(size.x, 1.0f);
        desc.Height = std::max(size.y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.IsBindless = false;

        desc.Format = RHI::RHIFormat::D32;
        desc.IsTypeless = true;
        desc.DebugName = "Depth Buffer";
        desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
        desc.InitialState = RHI::ResourceStates::DepthWrite;
        this->DepthTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);
        // -- Depth end ---

        // -- GBuffers ---
        desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
        desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
        desc.InitialState = RHI::ResourceStates::ShaderResource;
        desc.IsBindless = true;

        desc.Format = RHI::RHIFormat::RGBA32_FLOAT;
        desc.DebugName = "Albedo Buffer";
        this->AlbedoTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        // desc.Format = RHI::RHIFormat::R10G10B10A2_UNORM;
        desc.Format = RHI::RHIFormat::RGBA16_SNORM;
        desc.DebugName = "Normal Buffer";
        this->NormalTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.Format = RHI::RHIFormat::RGBA8_UNORM;
        desc.DebugName = "Surface Buffer";
        this->SurfaceTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.Format = RHI::RHIFormat::SRGBA8_UNORM;
        desc.DebugName = "Specular Buffer";
        this->SpecularTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.Format = RHI::RHIFormat::R10G10B10A2_UNORM;
        desc.DebugName = "Final Colour Buffer";
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::ShaderResource | BindingFlags::UnorderedAccess;
        this->FinalColourBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        this->RenderPass = IGraphicsDevice::GPtr->CreateRenderPass(
            {
                .Attachments =
                {
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->AlbedoTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->NormalTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->SurfaceTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->SpecularTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .Type = RenderPassAttachment::Type::DepthStencil,
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->DepthTex,
                        .InitialLayout = RHI::ResourceStates::DepthWrite,
                        .SubpassLayout = RHI::ResourceStates::DepthWrite,
                        .FinalLayout = RHI::ResourceStates::DepthWrite
                    },
                }
            });
    }

    void Free(RHI::IGraphicsDevice* gfxDevice)
    {
        gfxDevice->DeleteRenderPass(this->RenderPass);
        gfxDevice->DeleteTexture(this->DepthTex);
        gfxDevice->DeleteTexture(this->AlbedoTex);
        gfxDevice->DeleteTexture(this->NormalTex);
        gfxDevice->DeleteTexture(this->SurfaceTex);
        gfxDevice->DeleteTexture(this->SpecularTex);
    }
};


// TODO: MOVE TO ENGINE AND UPDATE DEFERREED RENDERER
struct DeferredLightingPass
{
    struct Input
    {
        RHI::TextureHandle Depth;
        RHI::TextureHandle GBufferAlbedo;
        RHI::TextureHandle GBufferNormals;
        RHI::TextureHandle GBufferSurface;
        RHI::TextureHandle GBufferSpecular;

        RHI::TextureHandle OutputTexture;

        RHI::BufferHandle FrameBuffer;
        Shader::Camera* CameraData;

        void FillGBuffer(GBufferRenderTargets const& renderTargets)
        {
            this->Depth = renderTargets.DepthTex;
            this->GBufferAlbedo = renderTargets.AlbedoTex;
            this->GBufferNormals = renderTargets.NormalTex;
            this->GBufferSurface = renderTargets.SurfaceTex;
            this->GBufferSpecular = renderTargets.SpecularTex;
        }
    };

    DeferredLightingPass(IGraphicsDevice* gfxDevice, std::shared_ptr<Renderer::CommonPasses> commonPasses)
        : m_gfxDevice(gfxDevice)
        , m_commonPasses(commonPasses)
    {
    }

    void Initialize(Graphics::ShaderFactory& factory)
    {
        this->m_pixelShader = factory.CreateShader(
            "PhxEngine/DeferredLightingPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "DeferredLightingPS",
            });

        this->m_computeShader = factory.CreateShader(
            "PhxEngine/DeferredLightingCS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "DeferredLightingCS",
            });
    }

    void Render(ICommandList* commandList, Input const& input)
    {
        // Set Push Constants for input texture Data.
        if (!this->m_computePso.IsValid())
        {
            this->m_computePso = this->m_gfxDevice->CreateComputePipeline({
                    .ComputeShader = this->m_computeShader
                });
        }

        auto _ = commandList->BeginScopedMarker("Deferred Lighting Pass CS");

        RHI::GpuBarrier preBarriers[] =
        {
            RHI::GpuBarrier::CreateTexture(input.Depth, this->m_gfxDevice->GetTextureDesc(input.Depth).InitialState, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateTexture(input.OutputTexture, this->m_gfxDevice->GetTextureDesc(input.OutputTexture).InitialState, RHI::ResourceStates::UnorderedAccess),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

        // TODO: Change to match same syntax as GFX pipeline
        commandList->SetComputeState(this->m_computePso);

        commandList->BindConstantBuffer(1, input.FrameBuffer);
        commandList->BindDynamicConstantBuffer(2, *input.CameraData);
        commandList->BindDynamicDescriptorTable(
            4,
            {
                input.Depth,
                input.GBufferAlbedo,
                input.GBufferNormals,
                input.GBufferSurface,
                input.GBufferSpecular
            });
        commandList->BindDynamicUavDescriptorTable(5, { input.OutputTexture });

        auto& outputDesc = this->m_gfxDevice->GetTextureDesc(input.OutputTexture);

        Shader::DefferedLightingCSConstants push = {};
        push.DipatchGridDim =
        {
            outputDesc.Width / DEFERRED_BLOCK_SIZE_X,
            outputDesc.Height / DEFERRED_BLOCK_SIZE_Y,
        };
        push.MaxTileWidth = 16;

        commandList->Dispatch(
            push.DipatchGridDim.x,
            push.DipatchGridDim.y,
            1);

        RHI::GpuBarrier postTransition[] =
        {
            RHI::GpuBarrier::CreateTexture(input.Depth, RHI::ResourceStates::ShaderResource, this->m_gfxDevice->GetTextureDesc(input.Depth).InitialState),
            RHI::GpuBarrier::CreateTexture(input.OutputTexture, RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetTextureDesc(input.OutputTexture).InitialState),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));
    }

private:
    IGraphicsDevice* m_gfxDevice;
    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
    RHI::ShaderHandle m_pixelShader;
    RHI::ShaderHandle m_computeShader;
    RHI::ComputePipelineHandle m_computePso;
};

// TODO: MOVE TO ENGINE AND UPDATE DEFERREED RENDERER
struct GBufferFillPass
{
    void Initialize(Graphics::ShaderFactory& factory)
    {
        this->m_vertexShader = factory.CreateShader(
            "PhxEngine/GBufferPassVS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "GBufferPassVS",
            });

        this->m_pixelShader = factory.CreateShader(
            "PhxEngine/GBufferPassPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "GBufferPassPS",
            });
    }

    void WindowResized(IGraphicsDevice* gfxDevice)
    {
        gfxDevice->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
    }

    void BeginPass(IGraphicsDevice* gfxDevice, ICommandList* commandList, BufferHandle frameCB, Shader::Camera cameraData, GBufferRenderTargets const& gbufferRenderTargets)
    {
		commandList->BeginRenderPass(gbufferRenderTargets.RenderPass);

		if (!this->m_pipeline.IsValid())
		{
			this->m_pipeline = gfxDevice->CreateGraphicsPipeline(
				{
					.VertexShader = this->m_vertexShader,
					.PixelShader = this->m_pixelShader,
			        .RtvFormats = {
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.AlbedoTex).Format,
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.NormalTex).Format,
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.SurfaceTex).Format,
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.SpecularTex).Format,
			        },
			        .DsvFormat = { IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.DepthTex).Format }
				});
		}
        commandList->SetGraphicsPipeline(this->m_pipeline);

        RHI::Viewport v(gbufferRenderTargets.CanvasSize.x, gbufferRenderTargets.CanvasSize.y);
        commandList->SetViewports(&v, 1);

        RHI::Rect rec(LONG_MAX, LONG_MAX);
        commandList->SetScissors(&rec, 1);

        commandList->BindConstantBuffer(1, frameCB);

        // TODO: Create a camera const buffer as well
        commandList->BindDynamicConstantBuffer(2, cameraData);
    }

    void EndPass(ICommandList* commandList)
    {
        commandList->EndRenderPass();
    }
private:
    RHI::ShaderHandle m_vertexShader;
    RHI::ShaderHandle m_pixelShader;
    RHI::GraphicsPipelineHandle m_pipeline;

};

class Shadows : public ApplicationBase
{
private:

public:
    Shadows(IPhxEngineRoot* root)
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
        this->m_textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/Sponza/Sponza.gltf";
        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());
        this->m_gbufferFillPass.Initialize(*this->m_shaderFactory);
        this->m_deferredLightingPass = std::make_shared<DeferredLightingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_deferredLightingPass->Initialize(*this->m_shaderFactory);

        this->m_frameConstantBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
            .Usage = RHI::Usage::Default,
            .Binding = RHI::BindingFlags::ConstantBuffer,
            .InitialState = RHI::ResourceStates::ConstantBuffer,
            .SizeInBytes = sizeof(Shader::Frame),
            .DebugName = "Frame Constant Buffer",
            });

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

            indexUpload.Free();
            vertexUpload.Free();
        }

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
        this->m_renderTargets.Free(this->GetGfxDevice());
        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());
        this->m_gbufferFillPass.WindowResized(this->GetGfxDevice());
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->m_rotation += deltaTime.GetSeconds() * 1.1f;
        this->GetRoot()->SetInformativeWindowTitle("Example: Deferred Rendering", {});
    }

    void RenderScene() override
    {
        this->m_mainCamera.Width = this->GetRoot()->GetCanvasSize().x;
        this->m_mainCamera.Height = this->GetRoot()->GetCanvasSize().y;
        this->m_mainCamera.UpdateCamera();

        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        {
            auto _ = commandList->BeginScopedMarker("Preare Frame Data");
            this->PrepareRenderData(commandList, this->m_scene);
        }

        // The camera data doesn't match whats in the CPU and what's in the GPU.
        Shader::Camera cameraData = {};
        cameraData.ViewProjection = this->m_mainCamera.ViewProjection;
        cameraData.ViewProjectionInv = this->m_mainCamera.ViewProjectionInv;
        cameraData.ProjInv = this->m_mainCamera.ProjectionInv;
        cameraData.ViewInv = this->m_mainCamera.ViewInv;

        // Set up RenderData
        {

            auto _ = commandList->BeginScopedMarker("GBuffer Fill");

            this->m_gbufferFillPass.BeginPass(this->GetGfxDevice(), commandList, this->m_frameConstantBuffer, cameraData, this->m_renderTargets);
            auto view = this->m_scene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::TransformComponent>();
            uint32_t instanceCount = 0;
            for (auto e : view)
            {
                instanceCount++;
            }

            GPUAllocation instanceBufferAlloc =
                commandList->AllocateGpu(
                    sizeof(Shader::ShaderMeshInstancePointer) * (size_t)instanceCount,
                    sizeof(Shader::ShaderMeshInstancePointer));

            const DescriptorIndex instanceBufferDescriptorIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(instanceBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);
            Shader::ShaderMeshInstancePointer* pInstancePointerData = (Shader::ShaderMeshInstancePointer*)instanceBufferAlloc.CpuData;
            instanceCount = 0;
            for (auto e : view)
            {
                auto [meshInstance, transform] = view.get<Scene::MeshInstanceComponent, Scene::TransformComponent>(e);

                Shader::ShaderMeshInstancePointer shaderMeshPtr = {};
                shaderMeshPtr.Create(meshInstance.GlobalBufferIndex, 0);

                // Write into actual GPU-buffer:
                std::memcpy(pInstancePointerData + instanceCount, &shaderMeshPtr, sizeof(shaderMeshPtr)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
                instanceCount++;

                auto meshComponent= this->m_scene.GetRegistry().get<Scene::MeshComponent>(meshInstance.Mesh);
                commandList->BindIndexBuffer(meshComponent.IndexGpuBuffer);

                for (size_t i = 0; i < meshComponent.Surfaces.size(); i++)
                {
                    auto& materiaComp = this->m_scene.GetRegistry().get<Scene::MaterialComponent>(meshComponent.Surfaces[i].Material);

                    Shader::GeometryPassPushConstants pushConstant = {};
                    pushConstant.GeometryIndex = meshComponent.GlobalGeometryBufferIndex + i;
                    pushConstant.MaterialIndex = materiaComp.GlobalBufferIndex;
                    pushConstant.InstancePtrBufferDescriptorIndex = instanceBufferDescriptorIndex;
                    pushConstant.InstancePtrDataOffset = (uint32_t)(instanceBufferAlloc.Offset + instanceCount * sizeof(Shader::ShaderMeshInstancePointer));

                    commandList->BindPushConstant(0, pushConstant);

                    commandList->DrawIndexed(
                        meshComponent.Surfaces[i].NumIndices,
                        1,
                        meshComponent.Surfaces[i].IndexOffsetInMesh);
                }
            }

            this->m_gbufferFillPass.EndPass(commandList);
        }

		DeferredLightingPass::Input input = {};
		input.FillGBuffer(this->m_renderTargets);
		input.OutputTexture = this->m_renderTargets.FinalColourBuffer;
        input.FrameBuffer = this->m_frameConstantBuffer;
        input.CameraData = &cameraData;

		this->m_deferredLightingPass->Render(commandList, input);

        this->m_commonPasses->BlitTexture(
            commandList,
            this->m_renderTargets.FinalColourBuffer,
            commandList->GetRenderPassBackBuffer(),
            this->GetRoot()->GetCanvasSize());

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

private:
    void PrepareRenderData(ICommandList* commandList, Scene::Scene& scene)
    {
        scene.OnUpdate(this->m_commonPasses);

        GPUAllocation lightBufferAlloc =
            commandList->AllocateGpu(
                sizeof(Shader::ShaderLight) * Shader::SHADER_LIGHT_ENTITY_COUNT,
                sizeof(Shader::ShaderLight));
        GPUAllocation matrixBufferAlloc =
            commandList->AllocateGpu(
                sizeof(DirectX::XMMATRIX) * Shader::MATRIX_COUNT,
                sizeof(DirectX::XMMATRIX));

        Shader::ShaderLight* lightArray = (Shader::ShaderLight*)lightBufferAlloc.CpuData;
        DirectX::XMMATRIX* matrixArray = (DirectX::XMMATRIX*)matrixBufferAlloc.CpuData;

        Shader::ShaderLight* renderLight = lightArray;
        renderLight->SetType(Scene::LightComponent::kDirectionalLight);
        renderLight->SetRange(10.0f);
        renderLight->SetIntensity(10.0f);
        renderLight->SetFlags(Scene::LightComponent::kEnabled);
        renderLight->SetDirection({ 0.1, -0.2, 1.0f });
        renderLight->ColorPacked = Core::Math::PackColour({ 1.0f, 1.0f, 1.0f, 1.0f });
        renderLight->SetIndices(0);
        renderLight->SetNumCascades(0);
        renderLight->Position = { 0.0f, 0.0f, 0.0f };

        Shader::Frame frameData = {};
        // Move to Renderer...
        frameData.BrdfLUTTexIndex = scene.GetBrdfLutDescriptorIndex();
        frameData.LightEntityDescritporIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(lightBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);
        frameData.LightDataOffset = lightBufferAlloc.Offset;
        frameData.LightCount = 1;

        frameData.MatricesDescritporIndex = RHI::cInvalidDescriptorIndex;
        frameData.MatricesDataOffset = 0;
        frameData.SceneData = scene.GetShaderData();

        frameData.SceneData.AtmosphereData.AmbientColour = float3(0.4f, 0.0f, 0.4f);

        // Upload data
        RHI::GpuBarrier preCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->m_frameConstantBuffer, RHI::ResourceStates::ConstantBuffer, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(scene.GetGeometryBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(scene.GetMaterialBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        };
        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

        commandList->WriteBuffer(this->m_frameConstantBuffer, frameData);

        commandList->CopyBuffer(
            scene.GetInstanceBuffer(),
            0,
            scene.GetInstanceUploadBuffer(),
            0,
            scene.GetNumInstances() * sizeof(Shader::MeshInstance));

        commandList->CopyBuffer(
            scene.GetGeometryBuffer(),
            0,
            scene.GetGeometryUploadBuffer(),
            0,
            scene.GetNumGeometryEntries() * sizeof(Shader::Geometry));

        commandList->CopyBuffer(
            scene.GetMaterialBuffer(),
            0,
            scene.GetMaterialUploadBuffer(),
            0,
            scene.GetNumMaterialEntries() * sizeof(Shader::MaterialData));

        RHI::GpuBarrier postCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->m_frameConstantBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ConstantBuffer),
            RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(scene.GetGeometryBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(scene.GetMaterialBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
    }

private:
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::GraphicsPipelineHandle m_pipeline;
    Scene::Scene m_scene;
    Scene::CameraComponent m_mainCamera;

    RHI::BufferHandle m_frameConstantBuffer;

    GBufferRenderTargets m_renderTargets;
    GBufferFillPass m_gbufferFillPass;
    std::shared_ptr<DeferredLightingPass> m_deferredLightingPass;
    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;

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
    params.Name = "PhxEngine Example: Shadows";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {
        Shadows example(root.get());
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
