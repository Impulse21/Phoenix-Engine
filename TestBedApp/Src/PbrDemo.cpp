#include "PbrDemo.h"

#include <memory>

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/Asserts.h>

#include <Shaders/PbrDemoVS_compiled.h>
#include <Shaders/PbrDemoPS_compiled.h>

#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>


using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace DirectX;

namespace ShaderStructures
{
    struct DrawPushConstant
    {
        uint32_t InstanceBufferIndex;
        uint32_t GeometryIndex;
    };

    struct SceneData
    {
        DirectX::XMFLOAT4X4 ViewProjection;

        // -- 16-byte boundry ---

        DirectX::XMFLOAT3 CameraPosition;
        uint32_t _padding;

        // -- 16-byte boundry ---

        DirectX::XMFLOAT3 SunDirection;
        uint32_t _padding1;

        // -- 16-byte boundry ---

        DirectX::XMFLOAT3 SunColour;
        uint32_t IrradianceMapTexture;

        // -- 16-byte boundry ---

        uint32_t PreFilteredEnvMapTexIndex;
        uint32_t BrdfLUTTexIndex;
    };
}

namespace PBRBindingSlots
{
    enum : uint8_t
    {
        DrawPushConstant = 0,
        SceneDataCB,
        MeshInstanceDataSB,
        GeometryDataSB,
        MaterialDataSB,
        BindlessDescriptorTable
    };
}


void XM_CALLCONV ComputeMatrices(CXMMATRIX view, CXMMATRIX projection, XMFLOAT4X4& viewProjection)
{
    XMMATRIX m = view * projection;
    XMStoreFloat4x4(&viewProjection, XMMatrixTranspose(m));
}

void PbrDemo::LoadContent()
{

    this->GetCommandList()->Open();
    this->m_fs = std::make_shared<NativeFileSystem>();

    this->m_textureCache = std::make_shared<TextureCache>(
        this->GetGraphicsDevice(),
        this->m_fs);

    const std::string baseDir = "D:\\Users\\C.DiPaolo\\Development\\Phoenix-Engine\\Assets\\";

    this->m_skyboxTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\\Milkyway\\skybox.dds", true, this->GetCommandList());
    this->m_irradanceMap = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\\Milkyway\\irradiance_map.dds", true, this->GetCommandList());
    this->m_prefilteredMap = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\\Milkyway\\prefiltered_radiance_map.dds", true, this->GetCommandList());
    this->m_brdfLUT = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\BrdfLut.dds", true, this->GetCommandList());

    this->CreateRenderTargets();

    this->CreatePipelineStates();

    this->CreateScene();
    this->GetCommandList()->Close();

    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList(), true);

    this->m_viewMatrix = XMMatrixLookAtLH(
        XMLoadFloat3(&this->m_scene->GetMainCamera()->GetTranslation()),
        XMVectorSet(0, 0, 0, 1),
        XMVectorSet(0, 1, 0, 0));
}

void PbrDemo::RenderScene()
{
    TextureHandle backBuffer = this->GetGraphicsDevice()->GetBackBuffer();
    this->GetCommandList()->Open();
    {
        this->GetCommandList()->BeginScropedMarker("Clear Render Target");
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
        this->GetCommandList()->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });
        this->GetCommandList()->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0);
    }

    {
        ScopedMarker m = this->GetCommandList()->BeginScropedMarker("Main Render Pass");

        // Set PSO
        this->GetCommandList()->SetGraphicsPSO(this->m_geomtryPassPso);
        this->GetCommandList()->SetRenderTargets({ backBuffer }, this->m_depthBuffer);
        Viewport v(WindowWidth, WindowHeight);
        this->GetCommandList()->SetViewports(&v, 1);
        Rect rec(LONG_MAX, LONG_MAX);
        this->GetCommandList()->SetScissors(&rec, 1);

        ShaderStructures::SceneData sceneData = {};
        sceneData.BrdfLUTTexIndex = this->m_brdfLUT->GetDescriptorIndex();
        sceneData.IrradianceMapTexture = this->m_irradanceMap->GetDescriptorIndex();
        sceneData.PreFilteredEnvMapTexIndex = this->m_prefilteredMap->GetDescriptorIndex();

        sceneData.CameraPosition = this->m_scene->GetMainCamera()->GetTranslation();
        sceneData.SunColour = { 1.0f, 1.0f, 1.0f };
        sceneData.SunDirection = { 1.25, 1.0f, -1.0f };

        ComputeMatrices(
            this->m_viewMatrix,
            this->m_scene->GetMainCamera()->GetProjectionMatrix(),
            sceneData.ViewProjection);

        this->GetCommandList()->BindDynamicConstantBuffer(PBRBindingSlots::SceneDataCB, sceneData);
        this->GetCommandList()->BindDynamicStructuredBuffer(PBRBindingSlots::MeshInstanceDataSB, this->m_scene->GetMeshInstanceData());
        this->GetCommandList()->BindDynamicStructuredBuffer(PBRBindingSlots::GeometryDataSB, this->m_scene->GetGeometryData());
        this->GetCommandList()->BindDynamicStructuredBuffer(PBRBindingSlots::MaterialDataSB, this->m_scene->GetMaterialData());
        this->GetCommandList()->BindResourceTable(PBRBindingSlots::BindlessDescriptorTable);

        // Draw the Meshes
        for (auto& meshInstance : this->m_scene->GetSceneGraph()->GetMeshInstanceNodes())
        {
            const auto& mesh = meshInstance->GetMeshData();
            this->GetCommandList()->BindIndexBuffer(mesh->Buffers->IndexBuffer);

            for (size_t i = 0; i < mesh->Geometry.size(); i++)
            {
                ShaderStructures::DrawPushConstant constants;
                constants.InstanceBufferIndex = meshInstance->GetInstanceIndex();
                constants.GeometryIndex = i;

                this->GetCommandList()->BindPushConstant(PBRBindingSlots::DrawPushConstant, constants);

                this->GetCommandList()->DrawIndexed(
                    mesh->Geometry[i]->NumIndices,
                    1,
                    mesh->Geometry[i]->IndexOffsetInMesh);
            }
        }
    }

    {
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
    }

    this->GetCommandList()->Close();
    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList());
}

void PbrDemo::CreateRenderTargets()
{    
    // -- Create Depth Buffer ---
    {
        TextureDesc desc = {};
        desc.Format = EFormat::D32;
        desc.Width = WindowWidth;
        desc.Height = WindowHeight;
        desc.Dimension = TextureDimension::Texture2D;
        desc.DebugName = "Depth Buffer";
        Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
        desc.OptmizedClearValue = std::make_optional<Color>(clearValue);

        this->m_depthBuffer = this->GetGraphicsDevice()->CreateDepthStencil(desc);
    }
}

void PbrDemo::CreatePipelineStates()
{
    // Create Pipeline State
    {
        ShaderDesc shaderDesc = {};
        shaderDesc.DebugName = "PBR Vertex Shader";
        shaderDesc.ShaderType = EShaderType::Vertex;

        ShaderHandle vs = this->GetGraphicsDevice()->CreateShader(shaderDesc, gPbrDemoVS, sizeof(gPbrDemoVS)); // TOOD:

        shaderDesc.DebugName = "PBR Pixel Shader";
        shaderDesc.ShaderType = EShaderType::Pixel;
        ShaderHandle ps = this->GetGraphicsDevice()->CreateShader(shaderDesc, gPbrDemoPS, sizeof(gPbrDemoPS));

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
        psoDesc.RtvFormats.push_back(this->GetGraphicsDevice()->GetBackBuffer()->GetDesc().Format);

        // TODO: Work around to get things working. I really shouldn't even care making my stuff abstract. But here we are.
        Dx12::RootSignatureBuilder builder = {};

        builder.Add32BitConstants<0, 0>(sizeof(ShaderStructures::DrawPushConstant) / 4);
        builder.AddConstantBufferView<1, 0>();
        builder.AddShaderResourceView<0, 0>();
        builder.AddShaderResourceView<1, 0>();
        builder.AddShaderResourceView<2, 0>();
        builder.AddStaticSampler<0, 0>(
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            1U);
        builder.AddStaticSampler<1, 0>(
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        Dx12::DescriptorTable descriptorTable = {};
        auto range = this->GetGraphicsDevice()->GetNumBindlessDescriptors();
        descriptorTable.AddSRVRange<0, 100>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        descriptorTable.AddSRVRange<0, 101>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        descriptorTable.AddSRVRange<0, 102>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        builder.AddDescriptorTable(descriptorTable);

        psoDesc.RootSignatureBuilder = &builder;

        this->m_geomtryPassPso = this->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }
}

void PbrDemo::CreateScene()
{
    auto sceneGraph = std::make_shared<SceneGraph>();

    // Let's create Data
    // Create a material

    // TODO: Fix path to be relative
    const std::string baseDir = "D:\\Users\\C.DiPaolo\\Development\\Phoenix-Engine\\Assets\\";

    auto goldMaterial = sceneGraph->CreateMaterial();
    goldMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_albedo.png", true, this->GetCommandList());
    goldMaterial->NormalMapTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_normal-dx.png", false, this->GetCommandList());
    goldMaterial->UseMaterialTexture = false;
    goldMaterial->RoughnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_roughness.png", false, this->GetCommandList());
    goldMaterial->MetalnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_metallic.png", false, this->GetCommandList());

    auto floorMaterial = sceneGraph->CreateMaterial();
    floorMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\OldPlankFlooring\\old-plank-flooring4_basecolor.png", true, this->GetCommandList());
    floorMaterial->NormalMapTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\OldPlankFlooring\\old-plank-flooring4_normal.png", false, this->GetCommandList());
    floorMaterial->UseMaterialTexture = false;
    floorMaterial->RoughnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\OldPlankFlooring\\old-plank-flooring4_roughness.png", false, this->GetCommandList());
    floorMaterial->MetalnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\OldPlankFlooring\\old-plank-flooring4_metalness.png", false, this->GetCommandList());
    floorMaterial->AoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\OldPlankFlooring\\old-plank-flooring4_AO.png", false, this->GetCommandList());

    auto bambooMaterial = sceneGraph->CreateMaterial();
    bambooMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\BambooWood\\bamboo-wood-semigloss-albedo.png", true, this->GetCommandList());
    bambooMaterial->NormalMapTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\BambooWood\\bamboo-wood-semigloss-normal.png", false, this->GetCommandList());
    bambooMaterial->UseMaterialTexture = false;
    bambooMaterial->RoughnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\BambooWood\\bamboo-wood-semigloss-roughness.png", false, this->GetCommandList());
    bambooMaterial->MetalnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\BambooWood\\bamboo-wood-semigloss-metal.png", false, this->GetCommandList());
    bambooMaterial->AoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\BambooWood\\bamboo-wood-semigloss-ao.png", false, this->GetCommandList());

    // Create plane
    auto planeMesh = sceneGraph->CreateMesh();
    SceneGraphHelpers::InitializePlaneMesh(planeMesh, floorMaterial, 400.0f, 400.0f, true);

    auto planeNode = sceneGraph->CreateNode<MeshInstanceNode>();
    planeNode->SetMeshData(planeMesh);
    planeNode->SetName("Plane Node");
    sceneGraph->AttachNode(sceneGraph->GetRootNode(), planeNode);

    // Create Sphere
    auto sphereMesh = sceneGraph->CreateMesh();
    SceneGraphHelpers::InitializeSphereMesh(sphereMesh, goldMaterial, 1.0f, 16U, true);

    auto sphereNode = sceneGraph->CreateNode<MeshInstanceNode>();
    sphereNode->SetMeshData(sphereMesh);
    sphereNode->SetName("Sphere Node");
    sphereNode->SetTranslation({ 0.0f, 1.0f, 0.0f });

    sceneGraph->AttachNode(sceneGraph->GetRootNode(), sphereNode);

    // Create Cube
    auto cubeMesh = sceneGraph->CreateMesh();
    SceneGraphHelpers::InitalizeCubeMesh(cubeMesh, bambooMaterial, 2.0f, true);

    auto cubeNode = sceneGraph->CreateNode<MeshInstanceNode>();
    cubeNode->SetMeshData(cubeMesh);
    cubeNode->SetName("Cube Node");
    cubeNode->SetTranslation({ 1.0f, 1.0f, 0.0f });

    sceneGraph->AttachNode(sceneGraph->GetRootNode(), cubeNode);

    // Add a camera node
    auto cameraNode = sceneGraph->CreateNode<PerspectiveCameraNode>();
    cameraNode->SetTranslation({ 0.0f, 1.0f, -6.0f });
    cameraNode->SetName("Main Camera");
    cameraNode->SetYFov(XMConvertToRadians(45.0f));
    cameraNode->SetZNear(0.1f);
    cameraNode->SetZFar(100.0f);

    float aspectRatio =
        WindowWidth / static_cast<float>(WindowHeight);
    cameraNode->SetAspectRatio(aspectRatio);

    sceneGraph->AttachNode(sceneGraph->GetRootNode(), cameraNode);

    // Create SceneGraph
    this->m_scene = std::make_unique<Scene>(
        this->m_fs,
        this->m_textureCache,
        this->GetGraphicsDevice());
    
    this->m_scene->SetMainCamera(cameraNode);
    this->m_scene->SetSceneGraph(sceneGraph);

    this->m_scene->RefreshGpuBuffers(this->GetCommandList());
}
