
#include "PhxEngine.h"
#include "Core/EntryPoint.h"
#include "Core/Math.h"

#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_stdlib.h"
#include "ThirdParty/ImGui/imGuIZMO/imGuIZMOquat.h"

#include "Graphics/RenderPathComponent.h"
#include "Graphics/TextureCache.h"

#include "Scene/Scene.h"
#include "Scene/SceneLoader.h"

#include "Shaders/ShaderInteropStructures.h"

#include "Systems/ConsoleVarSystem.h"

#include <vector>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Scene;


AutoConsoleVar_Int CVAR_DeferredLightUseCompute("DeferredLightingCompute.checkbox", "Use compute Deferred lighting", 0, ConsoleVarFlags::EditCheckbox);

void TransposeMatrix(DirectX::XMFLOAT4X4 const& in, DirectX::XMFLOAT4X4* out)
{
    auto matrix = DirectX::XMLoadFloat4x4(&in);

    DirectX::XMStoreFloat4x4(out, DirectX::XMMatrixTranspose(matrix));
}

// TODO: Find a better way to do this
namespace RootParameters_GBuffer
{
    enum
    {
        PushConstant = 0,
        FrameCB,
        CameraCB,
    };
}

namespace RootParameters_FullQuad
{
    enum
    {
        TextureSRV = 0,
    };
}

namespace RootParameters_DeferredLightingFulLQuad
{
    enum
    {
        FrameCB = 0,
        CameraCB,
        Lights,
        GBuffer,
    };
}

struct GBuffer
{
    PhxEngine::RHI::TextureHandle AlbedoTexture;
    PhxEngine::RHI::TextureHandle NormalTexture;
    PhxEngine::RHI::TextureHandle SurfaceTexture;
};


class TestBedRenderPath : public RenderPathComponent
{
public:
    TestBedRenderPath(Application* app)
        : m_app(app)
    {}

    void OnAttachWindow() override;
    void Update(TimeStep const& ts) override;
    void Render() override;
    void Compose(RHI::CommandListHandle commandList) override;

private:
    void ConstructSceneRenderData(RHI::CommandListHandle commandList);
    void CreateRenderTargets();
    void CreatePSO();

    void ConstructDebugData();

    GBuffer& GetCurrentGBuffer() { return this->m_gBuffer[this->m_app->GetFrameCount() % this->m_gBuffer.size()]; }
    PhxEngine::RHI::TextureHandle& GetCurrentDeferredLightBuffer() 
    { 
        return this->m_deferredLightBuffers[this->m_app->GetFrameCount() % this->m_deferredLightBuffers.size()];
        // return this->m_deferredLightBuffers[0];
    }

    PhxEngine::RHI::TextureHandle& GetCurrentDepthBuffer()
    {
        return this->m_depthBuffers[this->m_app->GetFrameCount() % this->m_depthBuffers.size()];
        // return this->m_deferredLightBuffers[0];
    }

private:
    PhxEngine::Scene::Scene m_scene;
    Application* m_app;

    RHI::CommandListHandle m_commandList;

    // -- Scene CPU Buffers ---
    std::vector<Shader::Geometry> m_geometryCpuData;
    std::vector<Shader::MaterialData> m_materialCpuData;

    // Uploaded every frame....Could be improved upon.
    std::vector<Shader::ShaderLight> m_lightCpuData;

    // -- Scene GPU Buffers ---
    RHI::BufferHandle m_geometryGpuBuffers;
    RHI::BufferHandle m_materialGpuBuffers;

    PhxEngine::RHI::GraphicsPSOHandle m_gbufferPassPso;
    PhxEngine::RHI::GraphicsPSOHandle m_fullscreenQuadPso;
    PhxEngine::RHI::GraphicsPSOHandle m_deferredLightFullQuadPso;

    std::array<GBuffer, NUM_BACK_BUFFERS> m_gBuffer;
    std::array<PhxEngine::RHI::TextureHandle, NUM_BACK_BUFFERS> m_deferredLightBuffers;
    std::array<PhxEngine::RHI::TextureHandle, NUM_BACK_BUFFERS> m_depthBuffers;

private:
    std::shared_ptr<TextureCache> m_textureCache;

    // -- Debug Logic stuff ---
    enum class TestType : uint8_t
    {
        DeferredRenderer = 0,
    };

    TestType m_currentTest = TestType::DeferredRenderer;
    std::vector<const char*> m_testItems = 
        { "Deferred Renderer" };

    enum class DisplayRenderTarget : uint8_t
    {
        DeferredLight = 0,
        Colour,
        Normal,
        Surface,
        Depth
    };

    DisplayRenderTarget m_renderTarget = DisplayRenderTarget::Colour;
    std::vector<const char*> m_renderTargetNames =
    { 
        "Deferred Light Buffer",
        "Colour Buffer",
        "Normal Buffer",
        "Surface Buffer",
        "Depth Buffer",
    };

    std::vector<const char*> m_cameraNames;
    std::vector<ECS::Entity> m_cameraEntities;
    int m_selectedCamera = 0;


    std::vector<const char*> m_lightNames;
    std::vector<ECS::Entity> m_lightEntities;
    int m_selectedLight = 0;
};

class TestApp : public Application
{
public:
    void Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice) override;
};

PhxEngine::Core::Application* PhxEngine::Core::CreateApplication(CommandLineArgs args)
{
    return new TestApp();
}

void TestApp::Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice)
{
    Application::Initialize(graphicsDevice);

    // Attach our custom layer logic
    this->AttachRenderPath(std::make_shared<TestBedRenderPath>(this));
}

void TestBedRenderPath::OnAttachWindow()
{
    auto* graphicsDevice = this->m_app->GetGraphicsDevice();
    this->m_textureCache = std::make_shared<TextureCache>(graphicsDevice);

    auto sceneLoader = CreateGltfSceneLoader(graphicsDevice, this->m_textureCache);

    this->m_commandList = graphicsDevice->CreateCommandList();
    this->m_commandList->Open();

    // TODO: This is a hack to set camera's with and height during loading of scene. Look to correct this
    PhxEngine::Scene::Scene::GetGlobalCamera().Width = this->m_app->GetCanvas().Width;
    PhxEngine::Scene::Scene::GetGlobalCamera().Height = this->m_app->GetCanvas().Height;

    bool result = sceneLoader->LoadScene("..\\Assets\\Models\\Sponza_Intel\\Main\\NewSponza_Main_Blender_glTF.gltf", this->m_commandList, this->m_scene);
    // bool result = sceneLoader->LoadScene("..\\Assets\\Models\\Sponza_Intel\\Main\\Sponza_YUp.gltf", this->m_commandList, this->m_scene);
    if (!result)
    {
        throw std::runtime_error("Failed to load Scene");
    }

    this->ConstructSceneRenderData(this->m_commandList);

    this->m_commandList->Close();
    auto fenceValue = graphicsDevice->ExecuteCommandLists(this->m_commandList.get());

    // Reserve worst case light data
    this->m_lightCpuData.resize(Shader::LIGHT_BUFFER_SIZE);

    this->ConstructDebugData();
    this->CreateRenderTargets();
    this->CreatePSO();

    // TODO: Add wait on fence.
    graphicsDevice->WaitForIdle();
}

void TestBedRenderPath::Update(TimeStep const& ts)
{
    this->m_scene.RunMeshInstanceUpdateSystem();

    static bool pWindowOpen = true;
    ImGui::Begin("Testing Application", &pWindowOpen, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Test Type");
    static int selectedTestItem = 0;
    ImGui::Combo("Test Type", &selectedTestItem, this->m_testItems.data(), this->m_testItems.size());
    this->m_currentTest = static_cast<TestType>(selectedTestItem);

    ImGui::Spacing();

    ImGui::Separator();
    
    ImGui::Text("Display Rneder Target");
    static int selectedRT = 0;
    ImGui::Combo("Render Targets", &selectedRT, this->m_renderTargetNames.data(), this->m_renderTargetNames.size());
    this->m_renderTarget = static_cast<DisplayRenderTarget>(selectedRT);

    ImGui::Spacing();

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera Info:"))
    {
        ImGui::Combo("Camera", &this->m_selectedCamera, this->m_cameraNames.data(), this->m_cameraNames.size());

        ImGui::Indent();
        auto& cameraComponent = *this->m_scene.Cameras.GetComponent(this->m_cameraEntities[this->m_selectedCamera]);
        // ImGui::InputFloat3("Eye", &cameraComponent.Eye.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Eye", &cameraComponent.Eye.x, "%.3f");

        //ImGui::InputFloat3("At", &cameraComponent.At.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("At", &cameraComponent.At.x, "%.3f");

        ImGui::InputFloat3("Up", &cameraComponent.Up.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Width", &cameraComponent.Width, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Height", &cameraComponent.Height, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("ZNear", &cameraComponent.ZNear, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("ZFar", &cameraComponent.ZFar, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Fov", &cameraComponent.FoV, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
       
        cameraComponent.UpdateCamera();

        // Get Rotation
        auto& transformComponent = *this->m_scene.Transforms.GetComponent(this->m_cameraEntities[this->m_selectedCamera]);
        ImGui::Text("Transform:");
        ImGui::InputFloat3("Translation", &transformComponent.LocalScale.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("Rotation", &transformComponent.LocalRotation.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Scale", &transformComponent.LocalScale.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        auto cameraRot = transformComponent.GetRotation();
        quat qt(
            cameraRot.w,
            cameraRot.x,
            cameraRot.y,
            cameraRot.z);
        if (ImGui::gizmo3D("##gizmo1", qt /*, size,  mode */))
        { 
            //transformComponent.Rotate(DirectX::XMFLOAT4(qt.x, qt.y, qt.z, qt.w));
            //transformComponent.UpdateTransform();
            //
            //cameraComponent.TransformCamera(transformComponent);
            //cameraComponent.UpdateCamera();
        }

        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Light Info"))
    {
        ImGui::Combo("Lights", &this->m_selectedLight, this->m_lightNames.data(), this->m_lightNames.size());

        if (!this->m_lightEntities.empty())
        {
            ImGui::Indent();

            auto& lightComponent = *this->m_scene.Lights.GetComponent(this->m_lightEntities[this->m_selectedLight]);

            switch (lightComponent.Type)
            {
            case LightComponent::LightType::kDirectionalLight:
                ImGui::Text("Light Type: Directional");
                break;
            case LightComponent::LightType::kOmniLight:
                ImGui::Text("Light Type: Omni");
                break;
            case LightComponent::LightType::kSpotLight:
                ImGui::Text("Light Type: Spot");
                break;
            default:
                ImGui::Text("Light Type: UNKOWN");
                break;
			}

			bool isEnabled = lightComponent.IsEnabled();
			ImGui::Checkbox("Is Enabled", &isEnabled);
			lightComponent.SetEnabled(isEnabled);

            if (lightComponent.Type != LightComponent::LightType::kOmniLight)
            {
                // Direction is starting from origin, so we need to negate it
                Vec3 light(-lightComponent.Direction.x, -lightComponent.Direction.y, -lightComponent.Direction.z);
                // get/setLigth are helper funcs that you have ideally defined to manage your global/member objs
                if (ImGui::gizmo3D("##Dir1", light /*, size,  mode */))
                {
                    lightComponent.Direction = { -light.x, -light.y, -light.z };
                }
            }
            
            if (lightComponent.Type == LightComponent::LightType::kOmniLight || lightComponent.Type == LightComponent::LightType::kSpotLight)
            {
                ImGui::InputFloat3("Position", &lightComponent.Position.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
            }

			ImGui::ColorEdit3("Colour", &lightComponent.Colour.x);

			ImGui::Unindent();
		}
	}

    ImGui::End();
}

void TestBedRenderPath::Render()
{
    // TODO:
    // 1) Add White Texture for missing texture lookups
    // 2) Sort Drawcalls from Back to Front or Front to back.
    // 3) Group Drawcalls by material usage.

    Shader::Frame frameData = {};
    // frameData.BrdfLUTTexIndex = this->m_scene.BrdfLUT->GetDescriptorIndex();
    frameData.Scene.MaterialBufferIndex = this->m_materialGpuBuffers->GetDescriptorIndex();
    frameData.Scene.GeometryBufferIndex = this->m_geometryGpuBuffers->GetDescriptorIndex();
    frameData.Scene.NumLights = 0;


    // Update per frame resources
    {
        // TODO: I am here - just iterate over the lights.
        // Fill Light data
        this->m_lightCpuData.clear();
        for (int i = 0; i < this->m_scene.Lights.GetCount(); i++)
        {
            auto& lightComponent = this->m_scene.Lights[i];
            if (lightComponent.IsEnabled())
            {
                Shader::ShaderLight& renderLight = this->m_lightCpuData.emplace_back();
                renderLight.SetType(lightComponent.Type);
                renderLight.SetRange(lightComponent.Range);
                renderLight.SetEnergy(lightComponent.Energy);
                renderLight.SetFlags(lightComponent.Flags);
                renderLight.SetDirection(lightComponent.Direction);
                renderLight.ColorPacked = Math::PackColour(lightComponent.Colour);

                auto& transformComponent = *this->m_scene.Transforms.GetComponent(this->m_scene.Lights.GetEntity(i));
                renderLight.Position = transformComponent.GetPosition();

                frameData.Scene.NumLights++;
            }
        }
    }

    auto& cameraComponent = *this->m_scene.Cameras.GetComponent(this->m_cameraEntities[this->m_selectedCamera]);

    Shader::Camera cameraData = {};
    cameraData.CameraPosition = cameraComponent.Eye;
    TransposeMatrix(cameraComponent.ViewProjection, &cameraData.ViewProjection);
    TransposeMatrix(cameraComponent.ProjectionInv, &cameraData.ProjInv);
    TransposeMatrix(cameraComponent.ViewInv, &cameraData.ViewInv);

    // -- Update GPU Buffers ---
    this->m_commandList->Open();

    GBuffer& currentBuffer = this->GetCurrentGBuffer();

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Clear Render Targets");
        this->m_commandList->ClearDepthStencilTexture(this->GetCurrentDepthBuffer(), true, 1.0f, false, 0.0f);
        this->m_commandList->ClearTextureFloat(currentBuffer.AlbedoTexture, currentBuffer.AlbedoTexture->GetDesc().OptmizedClearValue.value());
        this->m_commandList->ClearTextureFloat(currentBuffer.NormalTexture, currentBuffer.NormalTexture->GetDesc().OptmizedClearValue.value());
        this->m_commandList->ClearTextureFloat(currentBuffer.SurfaceTexture, currentBuffer.SurfaceTexture->GetDesc().OptmizedClearValue.value());
    }

    {
        // TODO: Z-Prepasss
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque GBuffer Pass");

        // -- Prepare PSO ---
        this->m_commandList->SetRenderTargets(
            {
                currentBuffer.AlbedoTexture,
                currentBuffer.NormalTexture,
                currentBuffer.SurfaceTexture
            },
            this->GetCurrentDepthBuffer());
        this->m_commandList->SetGraphicsPSO(this->m_gbufferPassPso);

        RHI::Viewport v(this->m_app->GetCanvas().Width, this->m_app->GetCanvas().Height);
        this->m_commandList->SetViewports(&v, 1);

        RHI::Rect rec(LONG_MAX, LONG_MAX);
        this->m_commandList->SetScissors(&rec, 1);

        // -- Bind Data ---

        this->m_commandList->BindDynamicConstantBuffer(RootParameters_GBuffer::FrameCB, frameData);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_GBuffer::CameraCB, cameraData);

        // this->m_commandList->BindResourceTable(PBRBindingSlots::BindlessDescriptorTable);

        // -- Draw Meshes ---
        for (int i = 0; i < this->m_scene.MeshInstances.GetCount(); i++)
        {
            auto& meshInstanceComponent = this->m_scene.MeshInstances[i];

            if ((meshInstanceComponent.RenderBucketMask & RenderType::RenderType_Opaque) != RenderType::RenderType_Opaque)
            {
                ECS::Entity e = this->m_scene.MeshInstances.GetEntity(i);
                auto& meshComponent = *this->m_scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
                std::string modelName = "UNKNOWN";
                auto* nameComponent = this->m_scene.Names.GetComponent(meshInstanceComponent.MeshId);
                if (nameComponent)
                {
                    modelName = nameComponent->Name;
                }
                continue;
            }

            ECS::Entity e = this->m_scene.MeshInstances.GetEntity(i);
            auto& meshComponent = *this->m_scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
            auto& transformComponent = *this->m_scene.Transforms.GetComponent(e);
            this->m_commandList->BindIndexBuffer(meshComponent.IndexGpuBuffer);


            std::string modelName = "UNKNOWN";
            auto* nameComponent = this->m_scene.Names.GetComponent(meshInstanceComponent.MeshId);
            if (nameComponent)
            {
                modelName = nameComponent->Name;
            }

            auto scrope = this->m_commandList->BeginScopedMarker(modelName);
            for (size_t i = 0; i < meshComponent.Geometry.size(); i++)
            {
                Shader::GeometryPassPushConstants pushConstant;
                pushConstant.MeshIndex = this->m_scene.Meshes.GetIndex(meshInstanceComponent.MeshId);
                pushConstant.GeometryIndex = meshComponent.Geometry[i].GlobalGeometryIndex;
                TransposeMatrix(transformComponent.WorldMatrix, &pushConstant.WorldTransform);
                this->m_commandList->BindPushConstant(RootParameters_GBuffer::PushConstant, pushConstant);

                this->m_commandList->DrawIndexed(
                    meshComponent.Geometry[i].NumIndices,
                    1,
                    meshComponent.Geometry[i].IndexOffsetInMesh);
            }
        }
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque Deferred Lighting Pass");

        this->m_commandList->SetGraphicsPSO(this->m_deferredLightFullQuadPso);

        // A second upload........ not ideal
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::FrameCB, frameData);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::CameraCB, cameraData);

        // -- Lets go ahead and do the lighting pass now >.<
        this->m_commandList->BindDynamicStructuredBuffer(
            RootParameters_DeferredLightingFulLQuad::Lights,
            this->m_lightCpuData);

        this->m_commandList->SetRenderTargets({ this->GetCurrentDeferredLightBuffer() }, nullptr);


        auto& currentDepthBuffer = this->GetCurrentDepthBuffer();

		RHI::GpuBarrier preTransition[] =
		{
			RHI::GpuBarrier::CreateTexture(currentDepthBuffer, currentDepthBuffer->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateTexture(currentBuffer.AlbedoTexture, currentBuffer.AlbedoTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateTexture(currentBuffer.NormalTexture, currentBuffer.NormalTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateTexture(currentBuffer.SurfaceTexture, currentBuffer.SurfaceTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
		};

		this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preTransition, _countof(preTransition)));

        this->m_commandList->BindDynamicDescriptorTable(
            RootParameters_DeferredLightingFulLQuad::GBuffer,
            {
                currentDepthBuffer,
                currentBuffer.AlbedoTexture,
                currentBuffer.NormalTexture,
                currentBuffer.SurfaceTexture
            });
        this->m_commandList->Draw(3, 1, 0, 0);

		RHI::GpuBarrier postTransition[] =
		{
			RHI::GpuBarrier::CreateTexture(currentDepthBuffer, RHI::ResourceStates::ShaderResource, currentDepthBuffer->GetDesc().InitialState),
			RHI::GpuBarrier::CreateTexture(currentBuffer.AlbedoTexture, RHI::ResourceStates::ShaderResource, currentBuffer.AlbedoTexture->GetDesc().InitialState),
			RHI::GpuBarrier::CreateTexture(currentBuffer.NormalTexture, RHI::ResourceStates::ShaderResource, currentBuffer.NormalTexture->GetDesc().InitialState),
			RHI::GpuBarrier::CreateTexture(currentBuffer.SurfaceTexture, RHI::ResourceStates::ShaderResource, currentBuffer.SurfaceTexture->GetDesc().InitialState),
		};

        this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));
    }

    {
        // TODO: Post Process
    }

    this->m_commandList->Close();
    this->m_app->GetGraphicsDevice()->ExecuteCommandLists(this->m_commandList.get());
}

void TestBedRenderPath::Compose(RHI::CommandListHandle commandList)
{
    // -- Prepare PSO ---
    commandList->SetGraphicsPSO(this->m_fullscreenQuadPso);

    RHI::Viewport v(this->m_app->GetCanvas().Width, this->m_app->GetCanvas().Height);
    commandList->SetViewports(&v, 1);

    RHI::Rect rec(LONG_MAX, LONG_MAX);
    commandList->SetScissors(&rec, 1);

    RHI::TextureHandle handle;
    switch (this->m_renderTarget)
    {
    case DisplayRenderTarget::Normal:
        handle = this->GetCurrentGBuffer().NormalTexture;
        break;

    case DisplayRenderTarget::Surface:
        handle = this->GetCurrentGBuffer().SurfaceTexture;
        break;

    case DisplayRenderTarget::Depth:
        handle = this->GetCurrentDepthBuffer();
        break;

    case DisplayRenderTarget::Colour:
        handle = this->GetCurrentGBuffer().AlbedoTexture;
        break;

    case DisplayRenderTarget::DeferredLight:
    default:
        handle = this->GetCurrentDeferredLightBuffer();
        break;
    }


    RHI::ResourceStates originalState = this->m_renderTarget == DisplayRenderTarget::Depth
        ? RHI::ResourceStates::DepthWrite
        : RHI::ResourceStates::RenderTarget;

    commandList->TransitionBarrier(handle, originalState, RHI::ResourceStates::ShaderResource);

    commandList->BindDynamicDescriptorTable(RootParameters_FullQuad::TextureSRV, { handle });
    commandList->Draw(3, 1, 0, 0);

    commandList->TransitionBarrier(handle, RHI::ResourceStates::ShaderResource, originalState);
}

void TestBedRenderPath::ConstructSceneRenderData(RHI::CommandListHandle commandList)
{
    size_t numGeometry = 0ull;
    for (int i = 0; i < this->m_scene.Meshes.GetCount(); i++)
    {
        MeshComponent& mesh = this->m_scene.Meshes[i];
        mesh.CreateRenderData(this->m_app->GetGraphicsDevice(), commandList);

        // Determine the number of geometry data
        numGeometry += mesh.Geometry.size();
    }

    // Construct Geometry Data
    // TODO: Create a thread to collect geometry Count
    size_t geometryCounter = 0ull;
    if (this->m_geometryCpuData.size() != numGeometry)
    {
        this->m_geometryCpuData.resize(numGeometry);

        size_t globalGeomtryIndex = 0ull;
        for (int i = 0; i < this->m_scene.Meshes.GetCount(); i++)
        {
            MeshComponent& mesh = this->m_scene.Meshes[i];

            for (int j = 0; j < mesh.Geometry.size(); j++)
            {
                // Construct the Geometry data
                auto& geometryData = mesh.Geometry[j];
                geometryData.GlobalGeometryIndex = geometryCounter++;

                auto& geometryShaderData = this->m_geometryCpuData[geometryData.GlobalGeometryIndex];

                geometryShaderData.MaterialIndex = this->m_scene.Materials.GetIndex(geometryData.MaterialID);
                geometryShaderData.NumIndices = geometryData.NumIndices;
                geometryShaderData.NumVertices = geometryData.NumVertices;
                geometryShaderData.IndexOffset = geometryData.IndexOffsetInMesh;
                geometryShaderData.VertexBufferIndex = mesh.VertexGpuBuffer->GetDescriptorIndex();
                geometryShaderData.PositionOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
                geometryShaderData.TexCoordOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
                geometryShaderData.NormalOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
                geometryShaderData.TangentOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;
            }
        }

        // -- Create GPU Data ---
        RHI::BufferDesc desc = {};
        desc.DebugName = "Geometry Data";
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::SrvView | RHI::BufferMiscFlags::Raw;
        desc.StrideInBytes = sizeof(Shader::Geometry);
        desc.SizeInBytes = sizeof(Shader::Geometry) * this->m_geometryCpuData.size();

        this->m_geometryGpuBuffers = this->m_app->GetGraphicsDevice()->CreateBuffer(desc);

        // Upload Data
        commandList->TransitionBarrier(this->m_geometryGpuBuffers, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
        commandList->WriteBuffer(this->m_geometryGpuBuffers, this->m_geometryCpuData);
        commandList->TransitionBarrier(this->m_geometryGpuBuffers, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
    }

    // Construct Material Data
    if (this->m_scene.Materials.GetCount() != this->m_materialCpuData.size())
    {
        // Create Material Data
        this->m_materialCpuData.resize(this->m_scene.Materials.GetCount());

        for (int i = 0; i < this->m_scene.Materials.GetCount(); i++)
        {
            auto& gpuMaterial = this->m_materialCpuData[i];
            this->m_scene.Materials[i].PopulateShaderData(gpuMaterial);
        }

        // -- Create GPU Data ---
        RHI::BufferDesc desc = {};
        desc.DebugName = "Material Data";
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::SrvView | RHI::BufferMiscFlags::Raw;
        desc.StrideInBytes = sizeof(Shader::MaterialData);
        desc.SizeInBytes = sizeof(Shader::MaterialData) * this->m_materialCpuData.size();

        this->m_materialGpuBuffers = this->m_app->GetGraphicsDevice()->CreateBuffer(desc);

        // Upload Data
        commandList->TransitionBarrier(this->m_materialGpuBuffers, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
        commandList->WriteBuffer(this->m_materialGpuBuffers, this->m_materialCpuData);
        commandList->TransitionBarrier(this->m_materialGpuBuffers, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
    }
}

void TestBedRenderPath::CreateRenderTargets()
{
    // -- Depth ---
    RHI::TextureDesc desc = {};
    desc.Width = this->m_app->GetCanvas().Width;
    desc.Height = this->m_app->GetCanvas().Height;
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.IsBindless = false;

    desc.Format = RHI::FormatType::D32;
    desc.IsTypeless = true;
    desc.DebugName = "Depth Buffer";
    RHI::Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
    desc.OptmizedClearValue = std::make_optional<RHI::Color>(clearValue);
    desc.BindingFlags = desc.BindingFlags | RHI::BindingFlags::DepthStencil;
    desc.InitialState = RHI::ResourceStates::DepthWrite;

    for (size_t i = 0; i < this->m_depthBuffers.size(); i++)
    {
        this->m_depthBuffers[i] = this->m_app->GetGraphicsDevice()->CreateTexture(desc);
    }

    // -- GBuffers ---
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::RenderTarget;

    clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    desc.OptmizedClearValue = std::make_optional<RHI::Color>(clearValue);

    for (size_t i = 0; i < this->m_gBuffer.size(); i++)
    {
        desc.Format = RHI::FormatType::RGBA32_FLOAT;
        desc.DebugName = "Albedo Buffer";

        this->m_gBuffer[i].AlbedoTexture = this->m_app->GetGraphicsDevice()->CreateTexture(desc);

        // desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
        desc.Format = RHI::FormatType::RGBA16_SNORM;
        desc.DebugName = "Normal Buffer";
        this->m_gBuffer[i].NormalTexture = this->m_app->GetGraphicsDevice()->CreateTexture(desc);


        desc.Format = RHI::FormatType::RGBA8_UNORM;
        desc.DebugName = "Surface Buffer";
        this->m_gBuffer[i].SurfaceTexture = this->m_app->GetGraphicsDevice()->CreateTexture(desc);
    }

    for (size_t i = 0; i < this->m_deferredLightBuffers.size(); i++)
    {
        // TODO: Determine what format should be used here.
        desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
        desc.DebugName = "Deferred Lighting";
        this->m_deferredLightBuffers[i] = this->m_app->GetGraphicsDevice()->CreateTexture(desc);
    }
}

void TestBedRenderPath::CreatePSO()
{
    // TODO: Use the files directly
    {
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::VS_GBufferPass);
        psoDesc.PixelShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::PS_GBufferPass);
        psoDesc.InputLayout = nullptr;

        // psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::None;
        psoDesc.RtvFormats.push_back(this->m_gBuffer[0].AlbedoTexture->GetDesc().Format);
        psoDesc.RtvFormats.push_back(this->m_gBuffer[0].NormalTexture->GetDesc().Format);
        psoDesc.RtvFormats.push_back(this->m_gBuffer[0].SurfaceTexture->GetDesc().Format);
        psoDesc.DsvFormat = this->GetCurrentDepthBuffer()->GetDesc().Format;

        this->m_gbufferPassPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }

    {
        // TODO: Use the files directly
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::VS_FullscreenQuad);
        psoDesc.PixelShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::PS_FullscreenQuad);
        psoDesc.InputLayout = nullptr;
        psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::Front;
        psoDesc.DepthStencilRenderState.DepthTestEnable = false;
        psoDesc.RtvFormats.push_back(this->m_app->GetGraphicsDevice()->GetBackBuffer()->GetDesc().Format);

        this->m_fullscreenQuadPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);

        psoDesc.VertexShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::VS_DeferredLighting);
        psoDesc.PixelShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::PS_DeferredLighting);

        this->m_deferredLightFullQuadPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }
}

void TestBedRenderPath::ConstructDebugData()
{
    // Construct Camera Lists for UI selection
    this->m_cameraEntities.reserve(this->m_scene.Cameras.GetCount());
    this->m_cameraNames.reserve(this->m_scene.Cameras.GetCount());
    for (size_t i = 0; i < this->m_scene.Cameras.GetCount(); i++)
    {
        ECS::Entity e = this->m_scene.Cameras.GetEntity(i);
        auto* cameraName = this->m_scene.Names.GetComponent(e);
        if (cameraName)
        {
            this->m_cameraEntities.push_back(e);
            this->m_cameraNames.push_back(cameraName->Name.c_str());
        }
    };

    // Construct Light Entities
    this->m_cameraEntities.reserve(this->m_scene.Lights.GetCount());
    this->m_cameraNames.reserve(this->m_scene.Lights.GetCount());
    for (size_t i = 0; i < this->m_scene.Lights.GetCount(); i++)
    {
        ECS::Entity e = this->m_scene.Lights.GetEntity(i);
        auto* lightName = this->m_scene.Names.GetComponent(e);
        if (lightName)
        {
            this->m_lightEntities.push_back(e);
            this->m_lightNames.push_back(lightName->Name.c_str());
        }
    };
}
