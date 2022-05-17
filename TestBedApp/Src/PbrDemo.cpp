#include "PbrDemo.h"

#include <memory>

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/Asserts.h>

#include <Shaders/PbrDemoVS_compiled.h>
#include <Shaders/PbrDemoPS_compiled.h>

#include <Shaders/DepthPathVS_compiled.h>

#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>


#include <GLFW/glfw3.h>

#define SHADOW_DEPTH 100.5f

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace DirectX;

namespace RD
{
    class ShadowCamera
    {
    public:
        ShadowCamera() = default;

        void UpdateMatrix(
            XMVECTOR lightDirection,    // Direction parallel to light, in direction of travel
            XMVECTOR shadowCentre,      // Center location on far bounding plane of shadowed region
            XMVECTOR ShadowBounds,      // Width, height, and depth in world space represented by the shadow buffer
            uint32_t bufferWidth,       // Shadow buffer width
            uint32_t bufferHeight,      // Shadow buffer height--usually same as width
            uint32_t bufferPrecision   // Bit depth of shadow buffer--usually 16 or 24
            )
        {
            // Set look direction
            // DirectX::XMMatrixShadow()
        }

        // Used to transform world space to texture space for shadow sampling
        const XMMATRIX& GetShadowMatrix() const { return this->m_shadowMatrix; }

    private:
        XMMATRIX m_shadowMatrix;
    };
}
namespace PBRPassShaderStructures
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

        DirectX::XMFLOAT4X4 ShadowViewProjection;


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

namespace ShadowPassStructures
{
    struct DrawPushConstant
    {
        DirectX::XMFLOAT4X4 LightViewProjection;
        uint32_t InstanceBufferIndex;
        uint32_t GeometryIndex;
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
        ShadowMapTexture,
        BindlessDescriptorTable
    };
}

namespace ShadowPassBindingSlots
{
    enum : uint8_t
    {
        DrawPushConstant = 0,
        MeshInstanceDataSB,
        GeometryDataSB,
        BindlessDescriptorTable
    };
}

float GetAxisInput(GLFWgamepadstate const& state, int inputId)
{
    auto value = state.axes[inputId];
    return value > 0.2f || value < -0.2f
        ? value
        : 0.0f;
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

    this->m_skyboxTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\Serpentine_Valley\\output_skybox.dds", true, this->GetCommandList());
    this->m_irradanceMap = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\Serpentine_Valley\\output_irradiance.dds", true, this->GetCommandList());
    this->m_prefilteredMap = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\Serpentine_Valley\\output_radiance.dds", true, this->GetCommandList());
    this->m_brdfLUT = this->m_textureCache->LoadTexture(baseDir + "Textures\\IBL\\BrdfLut.dds", true, this->GetCommandList());

    this->CreateRenderTargets();
    this->CreateShadowMap();

    this->GetCommandList()->TransitionBarrier(this->m_shadowMap, ResourceStates::DepthWrite, ResourceStates::ShaderResource);

    this->CreatePipelineStates();

    this->CreateScene();
    this->GetCommandList()->Close();

    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList(), true);

    this->m_viewMatrix = XMMatrixLookAtLH(
        XMLoadFloat3(&this->m_scene->GetMainCamera()->GetTranslation()),
        XMVectorSet(0, 0, 0, 1),
        XMVectorSet(0, 1, 0, 0));

    this->m_sunProj = XMMatrixOrthographicLH(100.0f, 100.0f, 1.0f, 100.5f);
}

void PbrDemo::RenderScene()
{
    TextureHandle backBuffer = this->GetGraphicsDevice()->GetBackBuffer();
    this->GetCommandList()->Open();
    {
        this->GetCommandList()->BeginScopedMarker("Clear Render Target");
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
        this->GetCommandList()->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });
        this->GetCommandList()->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0);

        // Transition to Write State
        this->GetCommandList()->TransitionBarrier(this->m_shadowMap, ResourceStates::ShaderResource, ResourceStates::DepthWrite);
        this->GetCommandList()->ClearDepthStencilTexture(this->m_shadowMap, true, 1.0f, false, 0);
    }

    {
        ScopedMarker _ = this->GetCommandList()->BeginScopedMarker("Shadow Pass");
        
        this->GetCommandList()->SetGraphicsPSO(this->m_shadowMapPassPso);
        this->GetCommandList()->SetRenderTargets({}, this->m_shadowMap);

        Viewport v(this->m_shadowMap->GetDesc().Width, this->m_shadowMap->GetDesc().Height);
        this->GetCommandList()->SetViewports(&v, 1);
        Rect rec(LONG_MAX, LONG_MAX);
        this->GetCommandList()->SetScissors(&rec, 1);

        this->GetCommandList()->BindDynamicStructuredBuffer(ShadowPassBindingSlots::MeshInstanceDataSB, this->m_scene->GetMeshInstanceData());
        this->GetCommandList()->BindDynamicStructuredBuffer(ShadowPassBindingSlots::GeometryDataSB, this->m_scene->GetGeometryData());
        this->GetCommandList()->BindResourceTable(ShadowPassBindingSlots::BindlessDescriptorTable);

        // Draw the Meshes
        for (auto& meshInstance : this->m_scene->GetSceneGraph()->GetMeshInstanceNodes())
        {
            const auto& mesh = meshInstance->GetMeshData();
            this->GetCommandList()->BindIndexBuffer(mesh->Buffers->IndexGpuBuffer);

            for (size_t i = 0; i < mesh->Geometry.size(); i++)
            {
                ShadowPassStructures::DrawPushConstant constants;
                constants.InstanceBufferIndex = meshInstance->GetInstanceIndex();
                constants.GeometryIndex = i;

                // TODO: No need to upload this everytime.
                constants.LightViewProjection = this->m_sunViewProj;

                this->GetCommandList()->BindPushConstant(ShadowPassBindingSlots::DrawPushConstant, constants);

                this->GetCommandList()->DrawIndexed(
                    mesh->Geometry[i]->NumIndices,
                    1,
                    mesh->Geometry[i]->IndexOffsetInMesh);
            }
        }
    }

    {
        ScopedMarker m = this->GetCommandList()->BeginScopedMarker("Transition Shadow Map");
        this->GetCommandList()->TransitionBarrier(this->m_shadowMap, ResourceStates::DepthWrite, ResourceStates::ShaderResource);
    }

    {
        ScopedMarker m = this->GetCommandList()->BeginScopedMarker("Main Render Pass");

        // Set PSO
        this->GetCommandList()->SetGraphicsPSO(this->m_geomtryPassPso);
        this->GetCommandList()->SetRenderTargets({ backBuffer }, this->m_depthBuffer);
        Viewport v(WindowWidth, WindowHeight);
        this->GetCommandList()->SetViewports(&v, 1);
        Rect rec(LONG_MAX, LONG_MAX);
        this->GetCommandList()->SetScissors(&rec, 1);

        PBRPassShaderStructures::SceneData sceneData = {};
        sceneData.BrdfLUTTexIndex = this->m_brdfLUT->GetDescriptorIndex();
        sceneData.IrradianceMapTexture = this->m_irradanceMap->GetDescriptorIndex();
        sceneData.PreFilteredEnvMapTexIndex = this->m_prefilteredMap->GetDescriptorIndex();

        sceneData.CameraPosition = this->m_scene->GetMainCamera()->GetTranslation();
        sceneData.SunColour = { 1.0f, 1.0f, 1.0f };
        sceneData.SunDirection = this->m_sunDirection;
        // sceneData.SunDirection = { 0.0, 0.0, -1.0f };
        // sceneData.SunDirection = { 0.335837096, 0.923879147, -0.183468640 };

        ComputeMatrices(
            this->m_viewMatrix,
            this->m_scene->GetMainCamera()->GetProjectionMatrix(),
            sceneData.ViewProjection);

        sceneData.ShadowViewProjection = this->m_sunViewProj;

        this->GetCommandList()->BindDynamicConstantBuffer(PBRBindingSlots::SceneDataCB, sceneData);
        this->GetCommandList()->BindDynamicStructuredBuffer(PBRBindingSlots::MeshInstanceDataSB, this->m_scene->GetMeshInstanceData());
        this->GetCommandList()->BindDynamicStructuredBuffer(PBRBindingSlots::GeometryDataSB, this->m_scene->GetGeometryData());
        this->GetCommandList()->BindDynamicStructuredBuffer(PBRBindingSlots::MaterialDataSB, this->m_scene->GetMaterialData());
        this->GetCommandList()->BindDynamicDescriptorTable(PBRBindingSlots::ShadowMapTexture, { this->m_shadowMap });

        this->GetCommandList()->BindResourceTable(PBRBindingSlots::BindlessDescriptorTable);

        // Draw the Meshes
        for (auto& meshInstance : this->m_scene->GetSceneGraph()->GetMeshInstanceNodes())
        {
            const auto& mesh = meshInstance->GetMeshData();
            this->GetCommandList()->BindIndexBuffer(mesh->Buffers->IndexGpuBuffer);

            for (size_t i = 0; i < mesh->Geometry.size(); i++)
            {
                PBRPassShaderStructures::DrawPushConstant constants;
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

void PbrDemo::Update(double elapsedTime)
{
    // Temp Light control stuff
    GLFWgamepadstate state;

    static const XMVECTOR WorldUp = { 0.0f, 1.0f, 0.0f };
    static const XMVECTOR WorldNorth = XMVector3Normalize(XMVector3Cross(g_XMIdentityR0, WorldUp));
    static const XMVECTOR WorldEast = XMVector3Cross(WorldUp, WorldNorth);
    static const XMMATRIX worldBase(WorldEast, WorldUp, WorldNorth, g_XMIdentityR3);

    
    XMMATRIX sunViewProj =
    {
        -0.000187966, -2.08932e-05, 0.000111946, 0.0f,
        6.83272e-05, -5.74766e-05, 0.00030796, 0.0f,
        -2.98023e-12, -0.000327675, -6.11563e-05, 0.0f,
        0.53418, 0.471191, 0.153994, 1.0f
    };

    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
    {
        this->m_pitch += GetAxisInput(state, GLFW_GAMEPAD_AXIS_RIGHT_Y) * 0.01f;
        this->m_yaw += GetAxisInput(state, GLFW_GAMEPAD_AXIS_RIGHT_X) * 0.01f;

        // Max out pitich
        // this->m_pitch = XMMin(XM_PIDIV2, this->m_pitch);
        // this->m_pitch = XMMax(-XM_PIDIV2, this->m_pitch);

        if (this->m_pitch > XM_PI)
        {
            this->m_pitch -= XM_2PI;
        }
        else if (this->m_pitch <= -XM_PI)
        {
            this->m_pitch += XM_2PI;
        }

        if (this->m_yaw > XM_PI)
        {
            this->m_yaw -= XM_2PI;
        }
        else if (this->m_yaw <= -XM_PI)
        {
            this->m_yaw += XM_2PI;
        }

        XMMATRIX orientation = worldBase * XMMatrixRotationX(this->m_pitch) * XMMatrixRotationY(this->m_yaw);


        XMVECTOR sunDir = XMVector3TransformNormal(WorldNorth, orientation);
        // XMVECTOR sunDir = { 0.0f, -1.0f, 0.0f };
        XMStoreFloat3(&this->m_sunDirection, sunDir);
        //XMStoreFloat3(&this->m_sunDirection, { 0.0f, -1.0f, 0.0f });
        this->m_sunView = DirectX::XMMatrixLookAtLH(
            XMVectorNegate(sunDir) * 2,
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f });

    }

    /*
    ComputeMatrices(
        this->m_sunView,
        this->m_sunProj,
        this->m_sunViewProj);
        
    ComputeMatrices(
        this->m_viewMatrix,
        this->m_scene->GetMainCamera()->GetProjectionMatrix(),
        this->m_sunViewProj);


    ComputeMatrices(
        this->m_sunView,
        this->m_scene->GetMainCamera()->GetProjectionMatrix(),
        this->m_sunViewProj);


     XMStoreFloat4x4(&this->m_sunViewProj, XMMatrixTranspose(sunViewProj));
    */

    ComputeMatrices(
        this->m_sunView,
        this->m_sunProj,
        this->m_sunViewProj);

}

void PbrDemo::CreateRenderTargets()
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

void PbrDemo::CreateShadowMap()
{
    const uint32_t shadowMapRes = 1024;
	TextureDesc desc = {};
	desc.Format = EFormat::D32;
	desc.Width = shadowMapRes;
	desc.Height = shadowMapRes;
	desc.Dimension = TextureDimension::Texture2D;
	desc.DebugName = "Shadow Map";
	Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
	desc.OptmizedClearValue = std::make_optional<Color>(clearValue);

	this->m_shadowMap = this->GetGraphicsDevice()->CreateDepthStencil(desc);
}

void PbrDemo::CreatePipelineStates()
{
    // Create Pipeline State
    {
        ShaderDesc shaderDesc = {};
        shaderDesc.DebugName = "PBR Vertex Shader";
        shaderDesc.ShaderType = EShaderType::Vertex;

        ShaderHandle vs = this->GetGraphicsDevice()->CreateShader(shaderDesc, gPbrDemoVS, sizeof(gPbrDemoVS));

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

        builder.Add32BitConstants<0, 0>(sizeof(PBRPassShaderStructures::DrawPushConstant) / 4);
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

        // Shadow Map Sampler
        builder.AddStaticSampler<2, 0>(
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            16u,
            D3D12_COMPARISON_FUNC_LESS);

        
        Dx12::DescriptorTable textureTable = {};
        textureTable.AddSRVRange<10, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);
        builder.AddDescriptorTable(textureTable);

        Dx12::DescriptorTable bindlessDescriptorTable = {};
        auto range = this->GetGraphicsDevice()->GetNumBindlessDescriptors();
        bindlessDescriptorTable.AddSRVRange<0, 100>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        bindlessDescriptorTable.AddSRVRange<0, 101>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        bindlessDescriptorTable.AddSRVRange<0, 102>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        builder.AddDescriptorTable(bindlessDescriptorTable);

        psoDesc.RootSignatureBuilder = &builder;

        this->m_geomtryPassPso = this->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }

    // Create Shadow map pass
    {
        ShaderDesc shaderDesc = {};
        shaderDesc.DebugName = "Depth Vertex Shader";
        shaderDesc.ShaderType = EShaderType::Vertex;

        ShaderHandle vs = this->GetGraphicsDevice()->CreateShader(shaderDesc, gDepthPathVS, sizeof(gDepthPathVS));

        GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = vs;

        // Currently only one shadow map as there is only one light source
        psoDesc.DsvFormat = this->m_shadowMap->GetDesc().Format;
        // psoDesc.RasterRenderState.CullMode = RasterCullMode::Front;
        psoDesc.RasterRenderState.DepthBias = 100000;
        psoDesc.RasterRenderState.DepthBiasClamp = 0.0f;
        psoDesc.RasterRenderState.SlopeScaledDepthBias = 1.0f;
        psoDesc.RasterRenderState.DepthClipEnable = false;

        // TODO: Work around to get things working. I really shouldn't even care making my stuff abstract. But here we are.
        Dx12::RootSignatureBuilder builder = {};

        builder.Add32BitConstants<0, 0>(sizeof(ShadowPassStructures::DrawPushConstant) / 4);
        builder.AddShaderResourceView<0, 0>();
        builder.AddShaderResourceView<1, 0>();

        Dx12::DescriptorTable descriptorTable = {};
        auto range = this->GetGraphicsDevice()->GetNumBindlessDescriptors();
        descriptorTable.AddSRVRange<0, 100>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        builder.AddDescriptorTable(descriptorTable);

        psoDesc.RootSignatureBuilder = &builder;

        this->m_shadowMapPassPso = this->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
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
    goldMaterial->Ao = 1.0f;


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

    auto plasticMaterial = sceneGraph->CreateMaterial();
    plasticMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\scuffed-plastic\\scuffed-plastic8-alb.png", true, this->GetCommandList());
    plasticMaterial->NormalMapTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\scuffed-plastic\\scuffed-plastic-normal.png", false, this->GetCommandList());
    plasticMaterial->UseMaterialTexture = false;
    plasticMaterial->RoughnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\scuffed-plastic\\scuffed-plastic-rough.png", false, this->GetCommandList());
    plasticMaterial->MetalnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\scuffed-plastic\\scuffed-plastic-metal.png", false, this->GetCommandList());
    plasticMaterial->Ao = 1.0f;


    auto shinyMetal = sceneGraph->CreateMaterial();
    shinyMetal->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\worn-shiny-metal\\worn-shiny-metal-albedo.png", true, this->GetCommandList());
    shinyMetal->NormalMapTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\worn-shiny-metal\\worn-shiny-metal-Normal-dx.png", false, this->GetCommandList());
    shinyMetal->UseMaterialTexture = false;
    shinyMetal->RoughnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\worn-shiny-metal\\worn-shiny-metal-Roughness.png", false, this->GetCommandList());
    shinyMetal->MetalnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\worn-shiny-metal\\worn-shiny-metal-Metallic.png", false, this->GetCommandList());
    shinyMetal->AoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\worn-shiny-metal\\worn-shiny-metal-ao.png", false, this->GetCommandList());


    auto defaultMaterial = sceneGraph->CreateMaterial();
    // defaultMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Default\\checker.png", true, this->GetCommandList());
    defaultMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Default\\checkerboard.png", true, this->GetCommandList());
    defaultMaterial->Roughness = 1.0f;
    defaultMaterial->Metalness = 0.5f;
    defaultMaterial->Ao = 1.0f;

    // Create plane
    auto planeMesh = sceneGraph->CreateMesh();
    SceneGraphHelpers::InitializePlaneMesh(planeMesh, defaultMaterial, 10.0f, 10.0f, true);
    // SceneGraphHelpers::InitializePlaneMesh(planeMesh, floorMaterial, 10.0f, 10.0f, true);

    auto planeNode = sceneGraph->CreateNode<MeshInstanceNode>();
    planeNode->SetMeshData(planeMesh);
    planeNode->SetName("Plane Node");
    sceneGraph->AttachNode(sceneGraph->GetRootNode(), planeNode);

    {
        // Create Sphere
        auto sphereMesh = sceneGraph->CreateMesh();
        SceneGraphHelpers::InitializeSphereMesh(sphereMesh, goldMaterial, 1.0f, 16U, true);

        auto sphereNode = sceneGraph->CreateNode<MeshInstanceNode>();
        sphereNode->SetMeshData(sphereMesh);
        sphereNode->SetName("Gold Sphere Node");
        sphereNode->SetTranslation(DirectX::XMFLOAT3(-2.0f, 1.0f, 0.0f));

        sceneGraph->AttachNode(sceneGraph->GetRootNode(), sphereNode);
    }

    {
        // Create Sphere
        auto sphereMesh = sceneGraph->CreateMesh();
        SceneGraphHelpers::InitializeSphereMesh(sphereMesh, shinyMetal, 1.0f, 16U, true);

        auto sphereNode = sceneGraph->CreateNode<MeshInstanceNode>();
        sphereNode->SetMeshData(sphereMesh);
        sphereNode->SetName("Shiny Metal Sphere Node");
        sphereNode->SetTranslation(DirectX::XMFLOAT3(-2.0f, 1.0f, 2.0f));

        sceneGraph->AttachNode(sceneGraph->GetRootNode(), sphereNode);
    }

    {
        // Create Sphere
        auto sphereMesh = sceneGraph->CreateMesh();
        SceneGraphHelpers::InitializeSphereMesh(sphereMesh, plasticMaterial, 1.0f, 16U, true);

        auto sphereNode = sceneGraph->CreateNode<MeshInstanceNode>();
        sphereNode->SetMeshData(sphereMesh);
        sphereNode->SetName("plastic Sphere Node");
        sphereNode->SetTranslation(DirectX::XMFLOAT3(-2.0f, 1.0f, -2.0f));
        // sphereNode->SetTranslation(DirectX::XMFLOAT3(0.0f, 1.5f, 0.0f));

        sceneGraph->AttachNode(sceneGraph->GetRootNode(), sphereNode);
    }

    // Create Cube
    auto cubeMesh = sceneGraph->CreateMesh();
    SceneGraphHelpers::InitalizeCubeMesh(cubeMesh, bambooMaterial, 2.0f, true);
    // SceneGraphHelpers::InitalizeCubeMesh(cubeMesh, bambooMaterial, 2.0f, true);

    auto cubeNode = sceneGraph->CreateNode<MeshInstanceNode>();
    cubeNode->SetMeshData(cubeMesh);
    cubeNode->SetName("Cube Node");
    cubeNode->SetTranslation(DirectX::XMFLOAT3(2.0f, 1.0f, 0.0f ));

    sceneGraph->AttachNode(sceneGraph->GetRootNode(), cubeNode);

    // Add a camera node
    auto cameraNode = sceneGraph->CreateNode<PerspectiveCameraNode>();
    cameraNode->SetTranslation(DirectX::XMFLOAT3(0.0f, 10.0f, -10.0f ));
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
