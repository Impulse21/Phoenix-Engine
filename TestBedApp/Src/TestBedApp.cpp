#include "TestBedApp.h"

#include <PhxEngine/Core/Log.h>
#include <string>

using namespace PhxEngine;
using namespace PhxEngine::RHI;

void TestBedApp::LoadContent()
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

    // Create Pipeline State
    {
        ShaderDesc shaderDesc = {};
        shaderDesc.DebugName = "PBR Vertex Shader";
        shaderDesc.ShaderType = EShaderType::Vertex;
        ShaderHandle vs = this->GetGraphicsDevice()->CreateShader(shaderDesc, nullptr, 0); // TOOD:

        shaderDesc.DebugName = "PBR Pixel Shader";
        shaderDesc.ShaderType = EShaderType::Pixel;
        ShaderHandle ps = this->GetGraphicsDevice()->CreateShader(shaderDesc, nullptr, 0);


        // InputLayoutHandle inputLayout = this->GetGraphicsDevice()->CreateInputLayout(vertexAttributeDescs.data(), vertexAttributeDescs.size());
        InputLayoutHandle inputLayout = nullptr;

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

void TestBedApp::RenderScene()
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
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
    }

    this->GetCommandList()->Close();
    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList());
}
