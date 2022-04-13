#include "EditorLayer.h"

#include <PhxEngine/App/Application.h>
#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Renderer/SceneLoader.h>
#include <imgui.h>

// -- For Debug Write ---
#include <iostream>
#include <fstream>

#include <Shaders/GeometryPassVS_compiled.h>
#include <Shaders/GeometryPassPS_compiled.h>

using namespace PhxEngine::Editor;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::Renderer::New;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;


namespace GeometryPassRP
{
    enum
    {
        PushConstant,
        FrameCB,
        CameraCB,
        LightSB,
        BindlessTables,
    };
}

PhxEngine::Editor::EditorLayer::EditorLayer()
{
    RHI::CommandListDesc desc = {};
    desc.DebugName = "Editor CommandList";
    this->m_commandList = AppInstance->GetGraphicsDevice()->CreateCommandList(desc);
}

void PhxEngine::Editor::EditorLayer::OnAttach()
{
	this->m_fs = std::make_shared<NativeFileSystem>();
	std::shared_ptr<IFileSystem> assetsDirectory = std::make_shared<RelativeFileSystem>(this->m_fs, "Assets\\Models");

    this->m_textureCache = std::make_shared<TextureCache>(
        AppInstance->GetGraphicsDevice(),
        assetsDirectory);

    this->m_scene.GetGlobalCamera().At = { 1.0f, 1.0f, 0.0f };
    this->m_scene.GetGlobalCamera().Eye = { 0.0f, 1.0f, 0.0f };
    this->m_scene.GetGlobalCamera().Width = AppInstance->GetWindowWidth();
    this->m_scene.GetGlobalCamera().Height = AppInstance->GetWindowHeight();
    this->m_scene.GetGlobalCamera().UpdateCamera();

    this->m_cameraController = CreateDebugCameraController(this->m_scene.GetGlobalCamera());

    std::shared_ptr<IFileSystem> modelFileSystem = std::make_shared<RelativeFileSystem>(this->m_fs, "Assets\\Models");
    auto loader = CreateGltfSceneLoader(modelFileSystem, this->m_textureCache, AppInstance->GetGraphicsDevice());

    this->m_commandList->Open();

    // bool success = loader->LoadScene("Box\\Box.gltf", this->m_commandList, this->m_scene);
    bool success = loader->LoadScene("Sponza\\glTF\\Sponza.gltf", this->m_commandList, this->m_scene);

    if (!success)
    {
        this->m_commandList->Close();
        LOG_ERROR("Failed to load Scene");
        return;
    }

    // -- Construct a light entity ---
    auto omniLightEntity = this->m_scene.EntityCreateLight("Omni Light 1");
    LightComponent& omniLight = *this->m_scene.Lights.GetComponent(omniLightEntity);
    omniLight.Type = LightComponent::LightType::kOmniLight;
    omniLight.Colour = { 1.0f, 1.0f, 1.0f };
    omniLight.Energy = 5.0f;
    omniLight.Range = 5.0f;


    TransformComponent& omniLightPos = *this->m_scene.Transforms.GetComponent(omniLightEntity);
    DirectX::XMVECTOR translation = { 0.0f, 3.0f, 0.0f };
    omniLightPos.Translate(translation);
    omniLightPos.UpdateTransform();

    this->m_scene.RefreshGpuBuffers(AppInstance->GetGraphicsDevice(), this->m_commandList);

    this->m_scene.SkyboxTexture = this->m_textureCache->LoadTexture("..\\Textures\\IBL\\PaperMill_Ruins_E\\PaperMill_Skybox.dds", true, this->m_commandList);
    this->m_scene.IrradanceMap = this->m_textureCache->LoadTexture("..\\Textures\\IBL\\PaperMill_Ruins_E\\PaperMill_IrradianceMap.dds", true, this->m_commandList);
    this->m_scene.PrefilteredMap = this->m_textureCache->LoadTexture("..\\Textures\\IBL\\PaperMill_Ruins_E\\PaperMill_RadianceMap.dds", true, this->m_commandList);
    this->m_scene.BrdfLUT = this->m_textureCache->LoadTexture("..\\Textures\\IBL\\BrdfLut.dds", true, this->m_commandList);

    // Load UI data
    this->m_omniLightTex = this->m_textureCache->LoadTexture("..\\\Editor\\OmniLightIcon.png", false, this->m_commandList);

    this->m_commandList->Close();
    // TODO: Wait for load to finish.
    this->m_loadFence = AppInstance->GetGraphicsDevice()->ExecuteCommandLists(this->m_commandList, true);

    TextureDesc desc = {};
    desc.Format = EFormat::D32;
    desc.Width = AppInstance->GetWindowWidth();
    desc.Height = AppInstance->GetWindowHeight();
    desc.Dimension = TextureDimension::Texture2D;
    desc.DebugName = "Depth Buffer";
    Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
    desc.OptmizedClearValue = std::make_optional<Color>(clearValue);

    this->m_depthBuffer = AppInstance->GetGraphicsDevice()->CreateDepthStencil(desc);

    this->CreatePSO();

    return;
}

void PhxEngine::Editor::EditorLayer::OnUpdate(Core::TimeStep const& ts)
{
    this->m_cameraController->Update(ts);
}

void PhxEngine::Editor::EditorLayer::OnRender(RHI::TextureHandle& currentBuffer)
{
    this->m_commandList->Open();
    this->m_commandList->TransitionBarrier(currentBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
    this->m_commandList->ClearTextureFloat(currentBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });

    // Call A scene Renderer specific stuff
    {
        this->m_commandList->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0);

        // Set PSO
        this->m_commandList->SetGraphicsPSO(this->m_geometryPassPso);
        this->m_commandList->SetRenderTargets({ currentBuffer }, this->m_depthBuffer);
        Viewport v(AppInstance->GetWindowWidth(), AppInstance->GetWindowHeight());

        this->m_commandList->SetViewports(&v, 1);
        Rect rec(LONG_MAX, LONG_MAX);
        this->m_commandList->SetScissors(&rec, 1);

        // Set up Frame constant buffer
        Shader::Frame frame = {};
        frame.BrdfLUTTexIndex = this->m_scene.BrdfLUT.Get() ? this->m_scene.BrdfLUT->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
        frame.Scene = {};
        
        // TODO: Fustrum call lights and geometry

        this->m_scene.PopulateShaderSceneData(frame.Scene);
        this->m_commandList->BindDynamicConstantBuffer(GeometryPassRP::FrameCB, frame);

        Shader::Camera camera = {};
        camera.CameraPosition = this->m_scene.GetGlobalCamera().Eye;
        const auto transposedViewProj = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&this->m_scene.GetGlobalCamera().ViewProjection));
        DirectX::XMStoreFloat4x4(&camera.ViewProjection, transposedViewProj);
        this->m_commandList->BindDynamicConstantBuffer(GeometryPassRP::CameraCB, camera);

        this->m_commandList->BindResourceTable(GeometryPassRP::BindlessTables);

        // -- Create Light Data ---
        // Use a static here to prevent allocations per frame.
        thread_local static std::vector<Shader::ShaderLight> sLights(this->m_scene.Lights.GetCount());
        sLights.clear();
        for (int i = 0; i < this->m_scene.Lights.GetCount(); i++)
        {
            LightComponent& light = this->m_scene.Lights[i];
            ECS::Entity lightEntity = this->m_scene.Lights.GetEntity(i);
            TransformComponent& transform = *this->m_scene.Transforms.GetComponent(lightEntity);
            auto& shaderLight = sLights.emplace_back();
            shaderLight.SetType(light.Type);
            shaderLight.SetFlags(light.Type);
            shaderLight.SetRange(light.Range);
            shaderLight.SetEnergy(light.Energy);
            shaderLight.Position = transform.GetPosition();
            shaderLight.ColorPacked = Math::PackColour({ light.Colour.x, light.Colour.y, light.Colour.z, 1.0f });
        }

        this->m_commandList->BindDynamicStructuredBuffer<Shader::ShaderLight>(GeometryPassRP::LightSB, sLights);

        // Iterate the mesh instances
        {
            ScopedMarker m = this->m_commandList->BeginScropedMarker("Main Render Pass");

            for (int i = 0; i < this->m_scene.MeshInstances.GetCount(); i++)
            {
                auto& meshEntity = this->m_scene.MeshInstances[i].MeshId;
                auto instanceEntity = this->m_scene.MeshInstances.GetEntity(i);
                auto& instanceTransform = *this->m_scene.Transforms.GetComponent(instanceEntity);

                auto& mesh = *this->m_scene.Meshes.GetComponent(meshEntity);
                this->m_commandList->BindIndexBuffer(mesh.IndexGpuBuffer);

                for (int i = 0; i < mesh.Geometry.size(); i++)
                {
                    auto& geometry = mesh.Geometry[i];

                    Shader::GeometryPassPushConstants pushConstants = {};
                    pushConstants.GeometryIndex = geometry.GlobalGeometryIndex;

                    const auto transposedViewProj = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&instanceTransform.WorldMatrix));
                    DirectX::XMStoreFloat4x4(&pushConstants.WorldTransform, transposedViewProj);

                    this->m_commandList->BindPushConstant(GeometryPassRP::PushConstant, pushConstants);
                    this->m_commandList->DrawIndexed(
                        geometry.NumIndices,
                        1,
                        geometry.IndexOffsetInMesh);
                }
            }
        }
    }

    // Draw Debug World
    // Draw Lights
    
    this->m_commandList->TransitionBarrier(currentBuffer, ResourceStates::RenderTarget, ResourceStates::Present);

    this->m_commandList->Close();
    AppInstance->GetGraphicsDevice()->ExecuteCommandLists(this->m_commandList);
}


void EditorLayer::CreatePSO()
{
    // Create Pipeline State
    {
        ShaderDesc shaderDesc = {};
        shaderDesc.DebugName = "Geometry Vertex Shader";
        shaderDesc.ShaderType = EShaderType::Vertex;

        ShaderHandle vs = AppInstance->GetGraphicsDevice()->CreateShader(shaderDesc, gGeometryPassVS, sizeof(gGeometryPassVS));

        shaderDesc.DebugName = "Geometry Pixel Shader";
        shaderDesc.ShaderType = EShaderType::Pixel;
        ShaderHandle ps = AppInstance->GetGraphicsDevice()->CreateShader(shaderDesc, gGeometryPassPS, sizeof(gGeometryPassPS));

        std::vector<VertexAttributeDesc> attributeDesc =
        {
            { "POSITION", 0, EFormat::RGB32_FLOAT, 0, VertexAttributeDesc::SAppendAlignedElement, false},
        };

        // InputLayoutHandle inputLayout = this->GetGraphicsDevice()->CreateInputLayout(attributeDesc.data(), attributeDesc.size());
        InputLayoutHandle inputLayout = nullptr;

        GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = vs;
        psoDesc.PixelShader = ps;
        // psoDesc.InputLayout = inputLayout;
        // psoDesc.InputLayout = nullptr;
        psoDesc.DsvFormat = this->m_depthBuffer->GetDesc().Format;
        psoDesc.RtvFormats.push_back(AppInstance->GetGraphicsDevice()->GetBackBuffer()->GetDesc().Format);

        // psoDesc.RasterRenderState.FrontCounterClockwise = true;

        // TODO: Work around to get things working. I really shouldn't even care making my stuff abstract. But here we are.
        Dx12::RootSignatureBuilder builder = {};

        builder.Add32BitConstants<999, 0>(sizeof(Shader::GeometryPassPushConstants) / 4);
        builder.AddConstantBufferView<0, 0>(); // FrameCB
        builder.AddConstantBufferView<1, 0>(); // Camera CB
        builder.AddShaderResourceView<0, 0>(); // Lights SB

        // Default Sampler
        builder.AddStaticSampler<50, 0>(
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            1U);

        // BRDF Sampler
        builder.AddStaticSampler<51, 0>(
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        // Shadow Map Sampler
        builder.AddStaticSampler<52, 0>(
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            16u,
            D3D12_COMPARISON_FUNC_LESS);

        Dx12::DescriptorTable bindlessDescriptorTable = {};
        Renderer::FillBindlessDescriptorTable(AppInstance->GetGraphicsDevice(), bindlessDescriptorTable);
        builder.AddDescriptorTable(bindlessDescriptorTable);

        psoDesc.RootSignatureBuilder = &builder;

        this->m_geometryPassPso = AppInstance->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }
}

void PhxEngine::Editor::EditorLayer::LoadEditorResources()
{
}

void PhxEngine::Editor::EditorLayer::DrawSceneImages()
{
}

