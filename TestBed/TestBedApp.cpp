
#include "PhxEngine.h"
#include "Core/EntryPoint.h"
#include "Core/Math.h"

#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_stdlib.h"
#include "ThirdParty/ImGui/imGuIZMO/imGuIZMOquat.h"

#include "Graphics/RenderPathComponent.h"
#include "Graphics/TextureCache.h"
#include "Graphics/RectPacker.h"

#include "Scene/Scene.h"
#include "Scene/SceneLoader.h"

#include "Shaders/ShaderInteropStructures.h"

#include "Systems/ConsoleVarSystem.h"

#include <vector>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Scene;
using namespace DirectX;

namespace
{
    constexpr RHI::FormatType kShadowAtlasFormat = RHI::FormatType::D32;
    constexpr uint32_t kMaxShadowResolution2D = 1024U;
    constexpr uint32_t kMaxShadowResoltuionCube = 256U;
    constexpr uint32_t kNumShadowCascades = 3U;

    // const std::string scenePath = "..\\Assets\\Models\\Sponza_Intel\\Main\\NewSponza_Main_Blender_glTF.gltf";
    const std::string scenePath = "..\\Assets\\Models\\\MaterialScene\\MatScene.gltf";
    // const std::string scenePath = "..\\Assets\\Models\\CameraTestScene\\CameraTestScene.gltf";
    // const std::string scenePath = "..\\Assets\\Models\\My_Cornell\\MyCornell.gltf";
}

AutoConsoleVar_Int CVAR_EnableIBL("Renderer.EnableIBL.checkbox", "Enabled Image Based Lighting", 0, ConsoleVarFlags::EditCheckbox);
AutoConsoleVar_Int CVAR_EnableCSLighting("Renderer.EnableCSLigthing.checkbox", "Enable Compute Shader Lighting Pass", 0, ConsoleVarFlags::EditReadOnly);
AutoConsoleVar_Int CVAR_EnableShadows("Renderer.EnableShadows.checkbox", "Enable Shadows", 1, ConsoleVarFlags::EditCheckbox);
AutoConsoleVar_Float CVAR_CSM_ZMulti("Renderer.CSM.ZMultu.checkbox", "CSM Z Multiplier", 10.0f, ConsoleVarFlags::EditFloatDrag);
AutoConsoleVar_Int CVAR_CSM_RenderFromCascade("Renderer.CSM.RenderFromCascade.checkbox", "Render From Cascade", 0, ConsoleVarFlags::EditCheckbox);
AutoConsoleVar_Int CVAR_CSM_CascadeNum("Renderer.CSM.CascadeNum.checkbox", "Cascade", 0, ConsoleVarFlags::Advanced);

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

    PhxEngine::RHI::TextureHandle _PostionTexture;
};


void DrawMeshes(PhxEngine::Scene::Scene const& scene, RHI::CommandListHandle commandList)
{
    auto scrope = commandList->BeginScopedMarker("Render Scene Meshes");

    for (int i = 0; i < scene.MeshInstances.GetCount(); i++)
    {
        auto& meshInstanceComponent = scene.MeshInstances[i];

        if ((meshInstanceComponent.RenderBucketMask & RenderType::RenderType_Opaque) != RenderType::RenderType_Opaque)
        {
            ECS::Entity e = scene.MeshInstances.GetEntity(i);
            auto& meshComponent = *scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
            std::string modelName = "UNKNOWN";
            auto* nameComponent = scene.Names.GetComponent(meshInstanceComponent.MeshId);
            if (nameComponent)
            {
                modelName = nameComponent->Name;
            }
            continue;
        }

        ECS::Entity e = scene.MeshInstances.GetEntity(i);
        auto& meshComponent = *scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
        auto& transformComponent = *scene.Transforms.GetComponent(e);
        commandList->BindIndexBuffer(meshComponent.IndexGpuBuffer);

        std::string modelName = "UNKNOWN";
        auto* nameComponent = scene.Names.GetComponent(meshInstanceComponent.MeshId);
        if (nameComponent)
        {
            modelName = nameComponent->Name;
        }

        auto scrope = commandList->BeginScopedMarker(modelName);
        for (size_t i = 0; i < meshComponent.Geometry.size(); i++)
        {
            Shader::GeometryPassPushConstants pushConstant;
            pushConstant.MeshIndex = scene.Meshes.GetIndex(meshInstanceComponent.MeshId);
            pushConstant.GeometryIndex = meshComponent.Geometry[i].GlobalGeometryIndex;
            TransposeMatrix(transformComponent.WorldMatrix, &pushConstant.WorldTransform);
            commandList->BindPushConstant(RootParameters_GBuffer::PushConstant, pushConstant);

            commandList->DrawIndexed(
                meshComponent.Geometry[i].NumIndices,
                1,
                meshComponent.Geometry[i].IndexOffsetInMesh);
        }
    }
}


namespace ShadowMaps
{
    std::vector<DirectX::XMVECTOR> GetFrustumCoordWorldSpace(DirectX::XMMATRIX const& invViewProj)
    {
        return
        {
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, -1, 0, 1), invViewProj), // Near Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, -1, 1, 1), invViewProj), // Far Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, 1, 0, 1), invViewProj), // Near Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1,1, 1, 1), invViewProj), // Far Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, -1, 0, 1), invViewProj), // Near Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1,-1, 1, 1), invViewProj), // Far Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, 1, 0, 1), invViewProj), // Near Plane
            DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1,1, 1, 1), invViewProj), // Far Plane
        };
    }

    std::vector<DirectX::XMVECTOR> GetFrustumCoordWorldSpace(DirectX::XMMATRIX const& view, DirectX::XMMATRIX const& proj)
    {
        return GetFrustumCoordWorldSpace(DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixMultiply(view, proj)));
    }

    DirectX::XMMATRIX GetLightSpaceMatrix(CameraComponent const& camera, LightComponent& light, float nearPlane, const float farPlane)
    {
        const DirectX::XMMATRIX frustumProj = DirectX::XMMatrixPerspectiveFovRH(camera.FoV, 1.7f, std::max(nearPlane, 0.001f), farPlane);

        const auto corners = ShadowMaps::GetFrustumCoordWorldSpace(DirectX::XMLoadFloat4x4(&camera.View), frustumProj);

        DirectX::XMVECTOR center = DirectX::XMVectorZero();
        for (size_t i = 0; i < corners.size(); i++)
        {
            center = DirectX::XMVectorAdd(center, corners[i]);
        }
        center = center / static_cast<float>(corners.size());

        // Create Light View Matrix
        const auto lightView = DirectX::XMMatrixLookAtRH(center - DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&light.Direction)), center, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::min();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::min();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::min();

        // Move Frustum conrers from world space to Light view space and calculate it's bounding
        for (const auto& corner : corners)
        {
            DirectX::XMFLOAT3 lightViewFrustumCorner;
            DirectX::XMStoreFloat3(&lightViewFrustumCorner, DirectX::XMVector3Transform(corner, lightView));

            minX = std::min(minX, lightViewFrustumCorner.x);
            maxX = std::max(maxX, lightViewFrustumCorner.x);
            minY = std::min(minY, lightViewFrustumCorner.y);
            maxY = std::max(maxY, lightViewFrustumCorner.y);
            minZ = std::min(minZ, lightViewFrustumCorner.z);
            maxZ = std::max(maxZ, lightViewFrustumCorner.z);
        }

        // Tune this parameter according to the scene
        float zMult = CVAR_CSM_ZMulti.GetFloat();
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        auto lightProjection = DirectX::XMMatrixOrthographicOffCenterRH(minX, maxX, minY, maxY, minZ, maxZ);

        return lightView * lightProjection;
    }

    DirectX::XMMATRIX GetLightSpaceMatrix(CameraComponent const& camera, LightComponent& light, Span<const DirectX::XMVECTOR> frustumCornersWS, float nearPlane, const float farPlane)
    {
        assert(frustumCornersWS.Size() == static_cast<size_t>(8));
        const XMVECTOR cornersWS[] =
        {
            XMVectorLerp(frustumCornersWS[0], frustumCornersWS[1], nearPlane),
            XMVectorLerp(frustumCornersWS[0], frustumCornersWS[1], farPlane),
            XMVectorLerp(frustumCornersWS[2], frustumCornersWS[3], nearPlane),
            XMVectorLerp(frustumCornersWS[2], frustumCornersWS[3], farPlane),
            XMVectorLerp(frustumCornersWS[4], frustumCornersWS[5], nearPlane),
            XMVectorLerp(frustumCornersWS[4], frustumCornersWS[5], farPlane),
            XMVectorLerp(frustumCornersWS[6], frustumCornersWS[7], nearPlane),
            XMVectorLerp(frustumCornersWS[6], frustumCornersWS[7], farPlane),
        };

        DirectX::XMVECTOR center = DirectX::XMVectorZero();
        for (size_t i = 0; i < _countof(cornersWS); i++)
        {
            center = DirectX::XMVectorAdd(center, cornersWS[i]);
        }
        center = center / static_cast<float>(_countof(cornersWS));

        // Create Light View Matrix
        const auto lightView = DirectX::XMMatrixLookAtRH(center - DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&light.Direction)), center, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::min();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::min();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::min();

        // Move Frustum conrers from world space to Light view space and calculate it's bounding
        for (const auto& corner : cornersWS)
        {
            DirectX::XMFLOAT3 lightViewFrustumCorner;
            DirectX::XMStoreFloat3(&lightViewFrustumCorner, DirectX::XMVector3Transform(corner, lightView));

            minX = std::min(minX, lightViewFrustumCorner.x);
            maxX = std::max(maxX, lightViewFrustumCorner.x);
            minY = std::min(minY, lightViewFrustumCorner.y);
            maxY = std::max(maxY, lightViewFrustumCorner.y);
            minZ = std::min(minZ, lightViewFrustumCorner.z);
            maxZ = std::max(maxZ, lightViewFrustumCorner.z);
        }

        // Tune this parameter according to the scene
        float zMult = CVAR_CSM_ZMulti.GetFloat();
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        auto lightProjection = DirectX::XMMatrixOrthographicOffCenterRH(minX, maxX, minY, maxY, minZ, maxZ);

        return lightView * lightProjection;
    }
}

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
    void ShadowAtlasPacking();

private:
    void UpdateFrameRenderData(RHI::CommandListHandle cmdList, Shader::Frame const& frameData);
    void ConstructSceneRenderData(RHI::CommandListHandle commandList);
    void CreateRenderTargets();
    void CreatePSO();
    void CreateRenderResources();

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
    RHI::CommandListHandle m_computeCommandList;

    // -- Scene CPU Buffers ---
    std::vector<Shader::Geometry> m_geometryCpuData;
    std::vector<Shader::MaterialData> m_materialCpuData;

    // Uploaded every frame....Could be improved upon.
    std::vector<Shader::ShaderLight> m_lightCpuData;
    std::vector<Shader::ShaderLight> m_shadowLights;

    // -- Shadow Stuff ---
    RHI::TextureHandle m_shadowAtlas;

    // -- Scene GPU Buffers ---
    RHI::BufferHandle m_geometryGpuBuffers;
    RHI::BufferHandle m_materialGpuBuffers;

    PhxEngine::RHI::GraphicsPSOHandle m_shadowPassPso;
    PhxEngine::RHI::GraphicsPSOHandle m_gbufferPassPso;
    PhxEngine::RHI::GraphicsPSOHandle m_fullscreenQuadPso;
    PhxEngine::RHI::GraphicsPSOHandle m_deferredLightFullQuadPso;
    PhxEngine::RHI::ComputePSOHandle m_computeQueuePso;

    std::array<GBuffer, NUM_BACK_BUFFERS> m_gBuffer;
    std::array<PhxEngine::RHI::TextureHandle, NUM_BACK_BUFFERS> m_deferredLightBuffers;
    std::array<PhxEngine::RHI::TextureHandle, NUM_BACK_BUFFERS> m_depthBuffers;

    PhxEngine::RHI::TextureHandle m_testComputeTexture;

    enum ConstantBufferTypes
    {
        CB_Frame = 0,
        NumCB
    };
    std::array<RHI::BufferHandle, NumCB> m_constantBuffers;
    

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

    {
        RHI::TextureDesc desc = {};
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::UnorderedAccess;
        desc.Format = RHI::FormatType::R11G11B10_FLOAT;
        desc.InitialState = RHI::ResourceStates::UnorderedAccess;
        desc.Width = this->m_app->GetCanvas().Width;
        desc.Height = this->m_app->GetCanvas().Height;
        this->m_testComputeTexture = graphicsDevice->CreateTexture(desc);
    }
    auto sceneLoader = CreateGltfSceneLoader(graphicsDevice, this->m_textureCache);

    RHI::CommandListDesc commandLineDesc = {};
    commandLineDesc.QueueType = RHI::CommandQueueType::Compute;
    this->m_computeCommandList = graphicsDevice->CreateCommandList(commandLineDesc);

    this->m_commandList = graphicsDevice->CreateCommandList();
    this->m_commandList->Open();

    // TODO: This is a hack to set camera's with and height during loading of scene. Look to correct this
    PhxEngine::Scene::Scene::GetGlobalCamera().Width = this->m_app->GetCanvas().Width;
    PhxEngine::Scene::Scene::GetGlobalCamera().Height = this->m_app->GetCanvas().Height;

    bool result = sceneLoader->LoadScene(scenePath, this->m_commandList, this->m_scene);

    if (!result)
    {
        throw std::runtime_error("Failed to load Scene");
    }

#if false
    this->m_scene.SkyboxTexture = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\PaperMill_Ruins_E\\PaperMill_Skybox.dds", true, this->m_commandList);
    this->m_scene.IrradanceMap = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\PaperMill_Ruins_E\\PaperMill_IrradianceMap.dds", true, this->m_commandList);
    this->m_scene.PrefilteredMap = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\PaperMill_Ruins_E\\PaperMill_RadianceMap.dds", true, this->m_commandList);
#else

    this->m_scene.SkyboxTexture = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\Serpentine_Valley\\output_skybox.dds", true, this->m_commandList);
    this->m_scene.IrradanceMap = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\Serpentine_Valley\\output_irradiance.dds", true, this->m_commandList);
    this->m_scene.PrefilteredMap = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\Serpentine_Valley\\output_radiance.dds", true, this->m_commandList);
#endif 
    this->m_scene.BrdfLUT = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\BrdfLut.dds", true, this->m_commandList);

    this->ConstructSceneRenderData(this->m_commandList);

    this->m_commandList->Close();
    auto fenceValue = graphicsDevice->ExecuteCommandLists(this->m_commandList.get());

    // Reserve worst case light data
    this->m_lightCpuData.resize(Shader::LIGHT_BUFFER_SIZE);

    this->ConstructDebugData();
    this->CreateRenderResources();

    // TODO: Add wait on fence.
    graphicsDevice->WaitForIdle();
}

void TestBedRenderPath::Update(TimeStep const& ts)
{
    this->m_scene.RunMeshInstanceUpdateSystem();

    static bool pWindowOpen = true;
    ImGui::Begin("Testing Application", &pWindowOpen);

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

        ImGui::InputFloat3("Eye", &cameraComponent.Eye.x, "%.3f");
        ImGui::InputFloat3("Forward", &cameraComponent.Forward.x, "%.3f");

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

            for (int i = 0; i < this->m_scene.Lights.GetCount(); i++)
            {
                if (ImGui::TreeNode(this->m_lightNames[i]))
                {
                    auto& lightComponent = this->m_scene.Lights[i];
                    bool isEnabled = lightComponent.IsEnabled();
                    ImGui::Checkbox("Enabled", &isEnabled);
                    lightComponent.SetEnabled(isEnabled);

                    bool castsShadows = lightComponent.CastShadows();
                    ImGui::Checkbox("Cast Shadows", &castsShadows);
                    lightComponent.SetCastShadows(castsShadows);

                    if (ImGui::TreeNode("Extra Details"))
                    {
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

                        if (lightComponent.Type != LightComponent::LightType::kOmniLight)
                        {
                            ImGui::InputFloat3("Direction", &lightComponent.Direction.x, "%.3f");

                            // Direction is starting from origin, so we need to negate it
                            Vec3 light(lightComponent.Direction.x, lightComponent.Direction.y, -lightComponent.Direction.z);
                            // get/setLigth are helper funcs that you have ideally defined to manage your global/member objs
                            ImGui::Text("This is not working as expected. Do Not Use");
                            if (ImGui::gizmo3D("##Dir1", light /*, size,  mode */))
                            {
                                lightComponent.Direction = { light.x, light.y, -light.z };
                            }

                        }

                        if (lightComponent.Type == LightComponent::LightType::kOmniLight || lightComponent.Type == LightComponent::LightType::kSpotLight)
                        {
                            ImGui::InputFloat3("Position", &lightComponent.Position.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
                        }
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
            }

			ImGui::Unindent();
		}
	}

    ImGui::End();

    // Run System Update
    this->m_scene.UpdateLightsSystem();
}

void TestBedRenderPath::Render()
{
    // TODO:
    // 1) Add flag to toggle between Compute shader and Pixel Shader Deferred (Not First)
    // 2) Clean up how lights are enabled.
    // 3) Add Shadows
    // 4) Add Fustrum culling/Light Culling
    // 5) DDGI
    // 6) Optmize

    Shader::Frame frameData = {};
    frameData.BrdfLUTTexIndex = this->m_scene.BrdfLUT->GetDescriptorIndex();
    frameData.Scene.MaterialBufferIndex = this->m_materialGpuBuffers->GetDescriptorIndex();
    frameData.Scene.GeometryBufferIndex = this->m_geometryGpuBuffers->GetDescriptorIndex();

    if (CVAR_EnableIBL.Get())
    {
        frameData.Scene.IrradianceMapTexIndex = this->m_scene.IrradanceMap->GetDescriptorIndex();
        frameData.Scene.PreFilteredEnvMapTexIndex = this->m_scene.PrefilteredMap->GetDescriptorIndex();
    }
    else
    {

        frameData.Scene.IrradianceMapTexIndex = RHI::cInvalidDescriptorIndex;
        frameData.Scene.PreFilteredEnvMapTexIndex = RHI::cInvalidDescriptorIndex;
    }

    frameData.Scene.NumLights = 0;

    // Update per frame resources
    {
        if (CVAR_EnableShadows.Get())
        {
            this->ShadowAtlasPacking();
        }

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
        auto scrope = this->m_commandList->BeginScopedMarker("Prepare Frame Data");
        this->UpdateFrameRenderData(this->m_commandList, frameData);
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Clear Render Targets");
        this->m_commandList->ClearDepthStencilTexture(this->GetCurrentDepthBuffer(), true, 1.0f, false, 0.0f);
        this->m_commandList->ClearTextureFloat(currentBuffer.AlbedoTexture, currentBuffer.AlbedoTexture->GetDesc().OptmizedClearValue.value());
        this->m_commandList->ClearTextureFloat(currentBuffer.NormalTexture, currentBuffer.NormalTexture->GetDesc().OptmizedClearValue.value());
        this->m_commandList->ClearTextureFloat(currentBuffer.SurfaceTexture, currentBuffer.SurfaceTexture->GetDesc().OptmizedClearValue.value());
        this->m_commandList->ClearTextureFloat(currentBuffer._PostionTexture, currentBuffer._PostionTexture->GetDesc().OptmizedClearValue.value());

    }

    {
        // TODO: Z-Prepasss
    }

	if (CVAR_EnableShadows.Get())
	{
		auto scrope = this->m_commandList->BeginScopedMarker("Shadow Pass");
        if (this->m_shadowAtlas != nullptr)
        {
            this->m_commandList->ClearDepthStencilTexture(this->m_shadowAtlas, true, 1.0f, false, 0.0f);
        }

        // -- Prepare PSO ---
        this->m_commandList->SetRenderTargets(
            { },
            this->m_shadowAtlas);


        this->m_commandList->SetGraphicsPSO(this->m_shadowPassPso);

        this->m_commandList->BindConstantBuffer(RootParameters_GBuffer::FrameCB, this->m_constantBuffers[CB_Frame]);

        // Draw meshes per light
        // Bind light Buffer

        // Render Each active light
        this->m_shadowLights.clear();
        for (int i = 0; i < this->m_scene.Lights.GetCount(); i++)
        {
            auto& lightComponent = this->m_scene.Lights[i];
            if (!lightComponent.IsEnabled() || !lightComponent.CastShadows())
            {
                continue;
            }

            switch (lightComponent.Type)
            {
            case LightComponent::LightType::kDirectionalLight:
            {
                // Construct Cascade shadow Views
                const float nearPlane = cameraComponent.ZNear;
                const float farPlane = cameraComponent.ZFar;
                const float cascadeSplits[kNumShadowCascades + 1] =
                {
                    farPlane * 0.0f,  // Near Plane
                    farPlane * 0.01f, // Near mid plane -> 1% of the view frustrum
                    farPlane * 0.10f, // Far mind plane -> 10% of the view frustrum
                    farPlane * 1.0f   // Far Plane
                };

                // Is there a way to not create a new proj matrix per sub-frustum section??????
                // Decided to use a lerp between min max. See GetLightSpaceMatrix
                // DirectX::XMMATRIX frustumProj = DirectX::XMMatrixPerspectiveFovRH(camera.FoV, 1.7f, std::max(nearPlane, 0.001f), farPlane);
                // const auto frustumCornersWS = ShadowMaps::GetFrustumCoordWorldSpace(DirectX::XMLoadFloat4x4(&cameraComponent.ViewProjectionInv));

                for (size_t cascade = 0; cascade < kNumShadowCascades; cascade++)
                {
                    // Compute cascade sub-frustum in light-view-space from the main frustum corners:
                    const float splitNear = cascadeSplits[cascade];
                    const float splitFar = cascadeSplits[cascade + 1];

                    auto cascadeVPMatrix = 
                        ShadowMaps::GetLightSpaceMatrix(
                            cameraComponent,
                            lightComponent,
                            splitNear,
                            splitFar);

                    Shader::Camera cameraData = {};
                    DirectX::XMStoreFloat4x4(&cameraData.ViewProjection, DirectX::XMMatrixTranspose(cascadeVPMatrix));

                    this->m_commandList->BindDynamicConstantBuffer(RootParameters_GBuffer::CameraCB, cameraData);

                    RHI::Viewport viewport = {};
                    viewport.MinX = static_cast<float>(lightComponent.ShadowRect.x + cascade * lightComponent.ShadowRect.w);
                    viewport.MinY = static_cast<float>(lightComponent.ShadowRect.y);
                    viewport.MaxX = viewport.MinX + lightComponent.ShadowRect.w;
                    viewport.MaxY = viewport.MinY + lightComponent.ShadowRect.h;
                    viewport.MinZ = 0.0f;
                    viewport.MaxZ = 1.0f;

                    this->m_commandList->SetViewports(&viewport, 1);
                    RHI::Rect rec(LONG_MAX, LONG_MAX);
                    this->m_commandList->SetScissors(&rec, 1);

                    RHI::ScopedMarker _ = this->m_commandList->BeginScopedMarker("Directional Light Cascade: " + std::to_string(cascade));
                    DrawMeshes(this->m_scene, this->m_commandList);
                }

                break;
            }
            case LightComponent::LightType::kSpotLight:
                continue;
                break;

            case LightComponent::LightType::kOmniLight:
            {
                // float nearZ = 0.1f;
                float nearZ = 1.0f;
                float farZ = lightComponent.Range;

                DirectX::XMMATRIX shadowProj = DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV2, 1, nearZ, farZ);

                std::array<DirectX::XMFLOAT4X4, 6> cameraTransforms;

                // TODO: Clean all of this up
                static auto constructTransform = [&](DirectX::XMFLOAT4 const& rotation, DirectX::XMMATRIX const& shadowProj, DirectX::XMFLOAT4X4& outTransform)
                {
                    DirectX::XMVECTOR q = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&rotation));
                    DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(q);
                    const DirectX::XMVECTOR to = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rot);
                    const DirectX::XMVECTOR up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot);
                    auto viewMatrix = DirectX::XMMatrixLookToLH(DirectX::XMLoadFloat3(&lightComponent.Position), to, up);
                    DirectX::XMStoreFloat4x4(&outTransform, DirectX::XMMatrixTranspose(viewMatrix * shadowProj));
                };

                constructTransform(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), shadowProj, cameraTransforms[0]); // -z
                constructTransform(DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f), shadowProj, cameraTransforms[1]); // +z
                constructTransform(DirectX::XMFLOAT4(0, 0.707f, 0, 0.707f), shadowProj, cameraTransforms[2]); // -x
                constructTransform(DirectX::XMFLOAT4(0, -0.707f, 0, 0.707f), shadowProj, cameraTransforms[3]); // +x
                constructTransform(DirectX::XMFLOAT4(0.707f, 0, 0, 0.707f), shadowProj, cameraTransforms[4]); // -y
                constructTransform(DirectX::XMFLOAT4(-0.707f, 0, 0, 0.707f), shadowProj, cameraTransforms[5]); // +y

                // construct viewports
                for (size_t i = 0; i < cameraTransforms.size(); i++)
                {
                    RHI::ScopedMarker _ = this->m_commandList->BeginScopedMarker("Omni Light (View: " + std::to_string(i) + ")");

                    RHI::Viewport viewport = {};
                    viewport.MinX = static_cast<float>(lightComponent.ShadowRect.x + i * lightComponent.ShadowRect.w);
                    viewport.MinY = static_cast<float>(lightComponent.ShadowRect.y);
                    viewport.MaxX = viewport.MinX + lightComponent.ShadowRect.w;
                    viewport.MaxY = viewport.MinY + lightComponent.ShadowRect.h;
                    viewport.MinZ = 0.0f;
                    viewport.MaxZ = 1.0f;

                    this->m_commandList->SetViewports(&viewport, 1);
                    RHI::Rect rec(LONG_MAX, LONG_MAX);
                    this->m_commandList->SetScissors(&rec, 1);

                    Shader::Camera cameraData = {};
                    cameraData.ViewProjection = cameraTransforms[i];

                    // Reuse object buffer
                    this->m_commandList->BindDynamicConstantBuffer(RootParameters_GBuffer::CameraCB, cameraData);

                    DrawMeshes(this->m_scene, this->m_commandList);
                }
                break;
            }
            default:
                continue;
            }
        }
	}

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque GBuffer Pass");

        // -- Prepare PSO ---
        this->m_commandList->SetRenderTargets(
            {
                currentBuffer.AlbedoTexture,
                currentBuffer.NormalTexture,
                currentBuffer.SurfaceTexture,
                currentBuffer._PostionTexture,
            },
            this->GetCurrentDepthBuffer());
        this->m_commandList->SetGraphicsPSO(this->m_gbufferPassPso);

        RHI::Viewport v(this->m_app->GetCanvas().Width, this->m_app->GetCanvas().Height);
        this->m_commandList->SetViewports(&v, 1);

        RHI::Rect rec(LONG_MAX, LONG_MAX);
        this->m_commandList->SetScissors(&rec, 1);

        // -- Bind Data ---

        this->m_commandList->BindConstantBuffer(RootParameters_GBuffer::FrameCB, this->m_constantBuffers[CB_Frame]);
        // TODO: Create a camera const buffer as well
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_GBuffer::CameraCB, cameraData);

        // this->m_commandList->BindResourceTable(PBRBindingSlots::BindlessDescriptorTable);

        DrawMeshes(this->m_scene, this->m_commandList);
    }

    auto& currentDepthBuffer = this->GetCurrentDepthBuffer();

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque Deferred Lighting Pass");

        this->m_commandList->SetGraphicsPSO(this->m_deferredLightFullQuadPso);

        // A second upload........ not ideal
        this->m_commandList->BindConstantBuffer(RootParameters_DeferredLightingFulLQuad::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::CameraCB, cameraData);

        // -- Lets go ahead and do the lighting pass now >.<
        this->m_commandList->BindDynamicStructuredBuffer(
            RootParameters_DeferredLightingFulLQuad::Lights,
            this->m_lightCpuData);

        this->m_commandList->SetRenderTargets({ this->GetCurrentDeferredLightBuffer() }, nullptr);


		RHI::GpuBarrier preTransition[] =
		{
			RHI::GpuBarrier::CreateTexture(currentDepthBuffer, currentDepthBuffer->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateTexture(currentBuffer.AlbedoTexture, currentBuffer.AlbedoTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateTexture(currentBuffer.NormalTexture, currentBuffer.NormalTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateTexture(currentBuffer.SurfaceTexture, currentBuffer.SurfaceTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateTexture(currentBuffer._PostionTexture, currentBuffer._PostionTexture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource),
		};

		this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preTransition, _countof(preTransition)));

        this->m_commandList->BindDynamicDescriptorTable(
            RootParameters_DeferredLightingFulLQuad::GBuffer,
            {
                currentDepthBuffer,
                currentBuffer.AlbedoTexture,
                currentBuffer.NormalTexture,
                currentBuffer.SurfaceTexture,
                currentBuffer._PostionTexture,
            });
        this->m_commandList->Draw(3, 1, 0, 0);

        if (!CVAR_EnableCSLighting.Get())
        {
            RHI::GpuBarrier postTransition[] =
            {
                RHI::GpuBarrier::CreateTexture(currentDepthBuffer, RHI::ResourceStates::ShaderResource, currentDepthBuffer->GetDesc().InitialState),
                RHI::GpuBarrier::CreateTexture(currentBuffer.AlbedoTexture, RHI::ResourceStates::ShaderResource, currentBuffer.AlbedoTexture->GetDesc().InitialState),
                RHI::GpuBarrier::CreateTexture(currentBuffer.NormalTexture, RHI::ResourceStates::ShaderResource, currentBuffer.NormalTexture->GetDesc().InitialState),
                RHI::GpuBarrier::CreateTexture(currentBuffer.SurfaceTexture, RHI::ResourceStates::ShaderResource, currentBuffer.SurfaceTexture->GetDesc().InitialState),
                RHI::GpuBarrier::CreateTexture(currentBuffer._PostionTexture, RHI::ResourceStates::ShaderResource, currentBuffer._PostionTexture->GetDesc().InitialState),
            };

            this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));

            {
                // TODO: Post Process
            }
        }
    }

    if (CVAR_EnableCSLighting.Get())
    {

        // Wait For Complition
        {
            this->m_commandList->Close();

            auto executionRecipt = this->m_app->GetGraphicsDevice()->ExecuteCommandLists(
                this->m_commandList.get());


            this->m_app->GetGraphicsDevice()->QueueWaitForCommandList(RHI::CommandQueueType::Compute, executionRecipt);
        }

        {
            this->m_computeCommandList->Open();
            auto scrope = this->m_computeCommandList->BeginScopedMarker("Opaque Deferred Lighting Pass");
            this->m_computeCommandList->SetComputeState(this->m_computeQueuePso);


            this->m_commandList->BindConstantBuffer(RootParameters_DeferredLightingFulLQuad::FrameCB + 1, this->m_constantBuffers[CB_Frame]);
            this->m_computeCommandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::CameraCB + 1, cameraData);

            // -- Lets go ahead and do the lighting pass now >.<
            this->m_computeCommandList->BindDynamicStructuredBuffer(
                RootParameters_DeferredLightingFulLQuad::Lights + 1,
                this->m_lightCpuData);

            auto& currentDepthBuffer = this->GetCurrentDepthBuffer();

            this->m_computeCommandList->BindDynamicDescriptorTable(
                RootParameters_DeferredLightingFulLQuad::GBuffer + 1,
                {
                    currentDepthBuffer,
                    currentBuffer.AlbedoTexture,
                    currentBuffer.NormalTexture,
                    currentBuffer.SurfaceTexture,
                    currentBuffer._PostionTexture,
                });

            // Bind UAVS
            this->m_computeCommandList->BindDynamicUavDescriptorTable(RootParameters_DeferredLightingFulLQuad::GBuffer + 2, { this->m_testComputeTexture });


            Shader::DefferedLightingCSConstants push = {};
            push.DipatchGridDim =
            {
                this->m_testComputeTexture->GetDesc().Width / DEFERRED_BLOCK_SIZE_X,
                this->m_testComputeTexture->GetDesc().Height / DEFERRED_BLOCK_SIZE_Y,
            };
            push.MaxTileWidth = 16;

            this->m_computeCommandList->BindPushConstant(0, push);

            // The texture width divided by the thread group size.
            this->m_computeCommandList->Dispatch(
                push.DipatchGridDim.x,
                push.DipatchGridDim.y,
                1);

            push.MaxTileWidth = 8;

            this->m_computeCommandList->BindPushConstant(0, push);

            // The texture width divided by the thread group size.
            this->m_computeCommandList->Dispatch(
                push.DipatchGridDim.x,
                push.DipatchGridDim.y,
                1);
        }

        this->m_computeCommandList->Close();
        this->m_app->GetGraphicsDevice()->ExecuteCommandLists(
            this->m_computeCommandList.get(),
            true,
            RHI::CommandQueueType::Compute);

        this->m_commandList->Open();
        RHI::GpuBarrier postTransition[] =
        {
            RHI::GpuBarrier::CreateTexture(currentDepthBuffer, RHI::ResourceStates::ShaderResource, currentDepthBuffer->GetDesc().InitialState),
            RHI::GpuBarrier::CreateTexture(currentBuffer.AlbedoTexture, RHI::ResourceStates::ShaderResource, currentBuffer.AlbedoTexture->GetDesc().InitialState),
            RHI::GpuBarrier::CreateTexture(currentBuffer.NormalTexture, RHI::ResourceStates::ShaderResource, currentBuffer.NormalTexture->GetDesc().InitialState),
            RHI::GpuBarrier::CreateTexture(currentBuffer.SurfaceTexture, RHI::ResourceStates::ShaderResource, currentBuffer.SurfaceTexture->GetDesc().InitialState),
            RHI::GpuBarrier::CreateTexture(currentBuffer._PostionTexture, RHI::ResourceStates::ShaderResource, currentBuffer._PostionTexture->GetDesc().InitialState),
        };

        this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));

        {
            // TODO: Post Process
        }

        this->m_commandList->Close();
        this->m_app->GetGraphicsDevice()->ExecuteCommandLists(this->m_commandList.get());
    }
    else
    {
        this->m_commandList->Close();
        this->m_app->GetGraphicsDevice()->ExecuteCommandLists(this->m_commandList.get());
    }
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

void TestBedRenderPath::UpdateFrameRenderData(RHI::CommandListHandle cmdList, Shader::Frame const& frameData)
{
    // Upload new data
    cmdList->TransitionBarrier(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::ConstantBuffer, RHI::ResourceStates::CopyDest);
    cmdList->WriteBuffer(this->m_constantBuffers[CB_Frame], frameData);
    cmdList->TransitionBarrier(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ConstantBuffer);
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
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
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
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
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

        desc.Format = RHI::FormatType::RGBA32_FLOAT;
        desc.DebugName = "Debug Position Buffer";
        this->m_gBuffer[i]._PostionTexture = this->m_app->GetGraphicsDevice()->CreateTexture(desc);
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
        psoDesc.RtvFormats.push_back(this->m_gBuffer[0]._PostionTexture->GetDesc().Format);
        psoDesc.DsvFormat = this->GetCurrentDepthBuffer()->GetDesc().Format;

        this->m_gbufferPassPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSO(psoDesc);
    }

    {
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::VS_ShadowPass);
        psoDesc.InputLayout = nullptr;

        psoDesc.DsvFormat = kShadowAtlasFormat;
        psoDesc.RasterRenderState.DepthBias = 100000;
        psoDesc.RasterRenderState.DepthBiasClamp = 0.0f;
        psoDesc.RasterRenderState.SlopeScaledDepthBias = 1.0f;
        psoDesc.RasterRenderState.DepthClipEnable = false;

        this->m_shadowPassPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSO(psoDesc);
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

        this->m_fullscreenQuadPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSO(psoDesc);

        psoDesc.VertexShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::VS_DeferredLighting);
        psoDesc.PixelShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::PS_DeferredLighting);

        this->m_deferredLightFullQuadPso = this->m_app->GetGraphicsDevice()->CreateGraphicsPSO(psoDesc);
    }

    {
        RHI::ComputePSODesc computeDesc = {};
        computeDesc.ComputeShader = this->m_app->GetShaderStore().Retrieve(PreLoadShaders::CS_DeferredLighting);
        this->m_computeQueuePso = this->m_app->GetGraphicsDevice()->CreateComputePso(computeDesc);
    }
}

void TestBedRenderPath::CreateRenderResources()
{
    this->CreateRenderTargets();
    this->CreatePSO();

    // -- Create Constant Buffers ---
    {
        RHI::BufferDesc bufferDesc = {};
        bufferDesc.CpuAccessMode = RHI::CpuAccessMode::Default;
        bufferDesc.Binding = RHI::BindingFlags::ConstantBuffer;
        bufferDesc.SizeInBytes = sizeof(Shader::Frame);
        bufferDesc.InitialState = RHI::ResourceStates::ConstantBuffer;

        bufferDesc.DebugName = "Frame Constant Buffer";
        this->m_constantBuffers[CB_Frame] = this->m_app->GetGraphicsDevice()->CreateBuffer(bufferDesc);
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

void TestBedRenderPath::ShadowAtlasPacking()
{
    thread_local static Graphics::RectPacker packer;
    packer.Clear();

    for (int i = 0; i < this->m_scene.Lights.GetCount(); i++)
    {
        auto& lightComponent = this->m_scene.Lights[i];

        if (!lightComponent.IsEnabled() || !lightComponent.CastShadows())
        {
            continue;
        }

        // use light characteristics to determine resolution.
        auto& camera = *this->m_scene.Cameras.GetComponent(this->m_cameraEntities[this->m_selectedCamera]);
        const float dist = Core::Math::Distance(camera.Eye, lightComponent.Position);
        const float range = lightComponent.Range;
        const float amount = std::min(1.0f, range / std::max(0.001f, dist));

        Graphics::PackerRect rect = {};
        rect.id = static_cast<int>(i);

        switch (lightComponent.Type)
        {
        case LightComponent::LightType::kDirectionalLight:
            rect.w = kMaxShadowResolution2D * static_cast<int>(kNumShadowCascades);
            rect.h = kMaxShadowResolution2D;
            break;

        case LightComponent::LightType::kSpotLight:
            rect.w = kMaxShadowResolution2D * amount;
            rect.h = kMaxShadowResolution2D * amount;
            break;

        case LightComponent::LightType::kOmniLight:

            rect.w = kMaxShadowResolution2D * amount * 6;
            rect.h = kMaxShadowResolution2D * amount;
            break;
        default:
            continue;
        }

        if (rect.w > 8 && rect.h > 8)
        {
            packer.AddRect(rect);
        }
    }

    if (!packer.IsEmpty())
    {
        // This limit is from Wicked Engine, not sure I know if it's arbitraty
        if (!packer.Pack(8192))
        {
            return;
        }
    
        for (const auto& rect : packer.GetRects())
        {
            uint32_t lightIndex = uint32_t(rect.id);
            LightComponent& light = this->m_scene.Lights[lightIndex];
            if (rect.was_packed)
            {
                light.ShadowRect = rect;

                // Remove slice multipliers from rect:
                switch (light.Type)
                {
                case LightComponent::kDirectionalLight:
                    light.ShadowRect.w /= static_cast<int>(kNumShadowCascades);
                    break;

                case LightComponent::kOmniLight:
                    light.ShadowRect.w /= 6;
                    break;
                }
            }
            else
            {
                light.Direction = {};
            }
        }

        if (this->m_shadowAtlas == nullptr ||
            (static_cast<int>(this->m_shadowAtlas->GetDesc().Width < packer.GetWidth() || 
                static_cast<int>(this->m_shadowAtlas->GetDesc().Height < packer.GetHeight()))))
        {
            RHI::TextureDesc desc;
            desc.Width = static_cast<uint32_t>(packer.GetWidth());
            desc.Height = static_cast<uint32_t>(packer.GetHeight());
            desc.Format = kShadowAtlasFormat;
            desc.IsTypeless = true;
            RHI::Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
            desc.OptmizedClearValue = std::make_optional<RHI::Color>(clearValue);

            desc.BindingFlags= RHI::BindingFlags::DepthStencil | RHI::BindingFlags::ShaderResource;
            desc.InitialState = RHI::ResourceStates::DepthWrite;
            desc.DebugName = "Shadow Map Atlas";
            this->m_shadowAtlas = this->m_app->GetGraphicsDevice()->CreateTexture(desc);
        }
    }
    // Determine the Texture size
}