#include "PbrDemo.h"

#include <memory>

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/Asserts.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

void PbrDemo::LoadContent()
{
    this->GetCommandList()->Open();
    this->m_fs = std::make_shared<NativeFileSystem>();

    this->m_textureCache = std::make_shared<TextureCache>(
        this->GetGraphicsDevice(),
        this->m_fs);

    this->CreateRenderTargets();

    this->CreatePipelineStates();

    this->CreateScene();
    this->GetCommandList()->Close();

    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList(), true);
}

void PbrDemo::RenderScene()
{
    auto* backBuffer = this->GetGraphicsDevice()->GetBackBuffer();
    this->GetCommandList()->Open();
    {
        this->GetCommandList()->BeginScropedMarker("Clear Render Target");
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
        this->GetCommandList()->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });
        this->GetCommandList()->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0);
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
        PHX_ASSERT(false); // TODO: Load shaders
        ShaderHandle vs = this->GetGraphicsDevice()->CreateShader(shaderDesc, nullptr, 0); // TOOD:

        shaderDesc.DebugName = "PBR Pixel Shader";
        shaderDesc.ShaderType = EShaderType::Pixel;
        PHX_ASSERT(false); // TODO: Load shaders
        ShaderHandle ps = this->GetGraphicsDevice()->CreateShader(shaderDesc, nullptr, 0);

        // Create Input Layout
        std::vector<VertexAttributeDesc> vertexAttributeDescs =
        {
            { "POSITION", EFormat::RGB32_FLOAT, 1, 0, 0, sizeof(float) * 3, false},
            { "NORMAL", EFormat::RGB32_FLOAT, 1, 0, 0, sizeof(float) * 3, false},
            { "COLOUR", EFormat::RGB32_FLOAT, 1, 0, 0, sizeof(float) * 3, false},
            { "TEXCOORD", EFormat::RG32_FLOAT, 1, 0, 0, sizeof(float) * 2, false},
            { "TANGENT", EFormat::RGBA32_FLOAT, 1, 0, 0, sizeof(float) * 4, false},
        };

        InputLayoutHandle inputLayout = this->GetGraphicsDevice()->CreateInputLayout(vertexAttributeDescs.data(), vertexAttributeDescs.size());

        GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = vs;
        psoDesc.PixelShader = ps;
        psoDesc.InputLayout = inputLayout;
        psoDesc.DsvFormat = this->m_depthBuffer->GetDesc().Format;
        psoDesc.RtvFormats.push_back(this->GetGraphicsDevice()->GetBackBuffer()->GetDesc().Format);

        ShaderParameterLayout shaderParameterLayout = {};
        shaderParameterLayout.AddPushConstantParmaeter(0, 0);
        shaderParameterLayout.AddSRVParameter(0);
        shaderParameterLayout.AddStaticSampler(
            0,
            true, true, true,
            SamplerAddressMode::Wrap);
        shaderParameterLayout.AddStaticSampler(
            1,
            true, true, true,
            SamplerAddressMode::Clamp);

        this->m_geomtryPassPso = this->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }
}

void PbrDemo::CreateScene()
{
    auto sceneGraph = std::make_shared<SceneGraph>();

    // Let's create Data
    // Create a material

    // TODO: Fix path to be relative
    const std::string baseDir = "D:\\Users\\C.DiPaolo\\Development\\Phoenix - Engine\\Assets\\";

    auto pMaterial = sceneGraph->CreateMaterial();
    pMaterial->AlbedoTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_albedo.png", true, this->GetCommandList());
    pMaterial->NormalMapTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_normal-dx.png", false, this->GetCommandList());
    pMaterial->UseMaterialTexture = false;
    pMaterial->RoughnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_roughness.png", false, this->GetCommandList());
    pMaterial->MetalnessTexture = this->m_textureCache->LoadTexture(baseDir + "Textures\\Materials\\Gold\\lightgold_metallic.png", false, this->GetCommandList());


    // Create SceneGraph
    this->m_scene = std::make_unique<Scene>(
        this->m_fs,
        this->m_textureCache,
        this->GetGraphicsDevice());
    
    this->m_scene->SetSceneGraph(sceneGraph);

}
