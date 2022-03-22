#include "ImGuiLayer.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <Shaders/ShaderInteropStructures.h>

#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>

#include <Shaders/ImGuiVS_compiled.h>
#include <Shaders/ImGuiPS_compiled.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

using namespace PhxEngine::Debug;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

enum RootParameters
{
    MatrixCB,           // cbuffer vertexBuffer : register(b0)
    BindlessResources,
    NumRootParameters
};

PhxEngine::Debug::ImGuiLayer::ImGuiLayer(
    RHI::IGraphicsDevice* graphicsDevice,
    GLFWwindow* gltfWindow)
	: Layer("ImGui Layer")
    , m_graphicsDevice(graphicsDevice)
    , m_gltfWindow(gltfWindow)
{
    this->m_commandlist = this->m_graphicsDevice->CreateCommandList();
}

void PhxEngine::Debug::ImGuiLayer::OnAttach()
{
    this->m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(this->m_imguiContext);

    if (!ImGui_ImplGlfw_InitForVulkan(this->m_gltfWindow, false))
    {
        LOG_CORE_ERROR("Failed to initalize IMGUI Window");
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontAllowUserScaling = true;

    unsigned char* pixelData = nullptr;
    int width;
    int height;

    io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

    // Create texture
    RHI::TextureDesc desc = {};
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.Width = width;
    desc.Height = height;
    desc.Format = RHI::EFormat::RGBA8_UNORM;
    desc.MipLevels = 1;
    desc.DebugName = "IMGUI Texture Font";

    this->m_fontTexture = this->m_graphicsDevice->CreateTexture(desc);

    RHI::SubresourceData subResourceData = {};

    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    this->m_commandlist->Open();
    this->m_commandlist->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    this->m_commandlist->WriteTexture(this->m_fontTexture, 0, 1, &subResourceData);
    this->m_commandlist->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);

    this->m_commandlist->Close();

    this->m_graphicsDevice->ExecuteCommandLists(this->m_commandlist);
    this->CreatePipelineStateObject();
}

void PhxEngine::Debug::ImGuiLayer::OnDetach()
{
    if (this->m_imguiContext)
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(this->m_imguiContext);
        this->m_imguiContext = nullptr;
    }
}

void PhxEngine::Debug::ImGuiLayer::Begin()
{
    ImGui::SetCurrentContext(this->m_imguiContext);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void PhxEngine::Debug::ImGuiLayer::End(RHI::CommandListHandle cmdList)
{
    auto scrope = cmdList->BeginScropedMarker("ImGui");

    ImGui::SetCurrentContext(this->m_imguiContext);
    ImGui::Render();

    ImGuiIO& io = ImGui::GetIO();
    ImDrawData* drawData = ImGui::GetDrawData();

    // Check if there is anything to render.
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return;
    }

    ImVec2 displayPos = drawData->DisplayPos;
    cmdList->SetGraphicsPSO(this->m_pso);
    cmdList->SetRenderTargets({ this->m_graphicsDevice->GetBackBuffer() }, nullptr);

    cmdList->BindResourceTable(RootParameters::BindlessResources);

    // Set root arguments.
    //    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixOrthographicRH( drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 1.0f );
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    float mvp[4][4] =
    {
        { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
        { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
        { 0.0f,         0.0f,           0.5f,       0.0f },
        { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
    };

    Shader::ImguiDrawInfo push = {};
    push.Mvp = DirectX::XMFLOAT4X4(&mvp[0][0]);
    push.TextureIndex = this->m_fontTexture->GetDescriptorIndex();

    cmdList->BindPushConstant(RootParameters::MatrixCB, push);
    Viewport v(drawData->DisplaySize.x, drawData->DisplaySize.y);
    cmdList->SetViewports(&v, 1);

    const EFormat indexFormat = sizeof(ImDrawIdx) == 2 ? EFormat::R16_UINT : EFormat::R32_UINT;

    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const ImDrawList* drawList = drawData->CmdLists[i];

        cmdList->BindDynamicVertexBuffer(0, drawList->VtxBuffer.size(), sizeof(ImDrawVert), drawList->VtxBuffer.Data);
        cmdList->BindDynamicIndexBuffer(drawList->IdxBuffer.size(), indexFormat, drawList->IdxBuffer.Data);

        int indexOffset = 0;
        for (int j = 0; j < drawList->CmdBuffer.size(); ++j)
        {
            const ImDrawCmd& drawCmd = drawList->CmdBuffer[j];
            if (drawCmd.UserCallback)
            {
                drawCmd.UserCallback(drawList, &drawCmd);
            }
            else
            {
                ImVec4 clipRect = drawCmd.ClipRect;
                Rect scissorRect;
                // TODO: Validate
                scissorRect.MinX = static_cast<int>(clipRect.x - displayPos.x);
                scissorRect.MinY = static_cast<int>(clipRect.y - displayPos.y);
                scissorRect.MaxX = static_cast<int>(clipRect.z - displayPos.x);
                scissorRect.MaxY = static_cast<int>(clipRect.w - displayPos.y);

                if (scissorRect.MaxX - scissorRect.MinX > 0.0f &&
                    scissorRect.MaxY - scissorRect.MinY > 0.0)
                {
                    cmdList->SetScissors(&scissorRect, 1);
                    cmdList->DrawIndexed(drawCmd.ElemCount, 1, indexOffset);
                }
            }
            indexOffset += drawCmd.ElemCount;
        }
    }

    ImGui::EndFrame();
}

void PhxEngine::Debug::ImGuiLayer::CreatePipelineStateObject()
{
    ShaderDesc shaderDesc = {};
    shaderDesc.DebugName = "ImGui_VS";
    shaderDesc.ShaderType = EShaderType::Vertex;
    ShaderHandle vs = this->m_graphicsDevice->CreateShader(shaderDesc, gImGuiVS, _countof(gImGuiVS));

    shaderDesc.DebugName = "ImGui_PS";
    shaderDesc.ShaderType = EShaderType::Pixel;
    ShaderHandle ps = this->m_graphicsDevice->CreateShader(shaderDesc, gImGuiPS, _countof(gImGuiPS));

    std::vector<VertexAttributeDesc> attributeDesc =
    {
        { "POSITION", 0, EFormat::RG32_FLOAT, 0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "TEXCOORD", 0, EFormat::RG32_FLOAT, 0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "COLOR", 0, EFormat::RGBA8_UNORM, 0, VertexAttributeDesc::SAppendAlignedElement, false},
    };

    InputLayoutHandle inputLayout = this->m_graphicsDevice->CreateInputLayout(attributeDesc.data(), attributeDesc.size());

    GraphicsPSODesc psoDesc = {};
    psoDesc.VertexShader = vs;
    psoDesc.PixelShader = ps;
    psoDesc.InputLayout = inputLayout;
    // TODO: Render to it's own resource, then compose image.
    psoDesc.RtvFormats.push_back(this->m_graphicsDevice->GetBackBuffer()->GetDesc().Format);

    auto& blendTarget = psoDesc.BlendRenderState.Targets[0];
    blendTarget.BlendEnable = true;
    blendTarget.SrcBlend = BlendFactor::SrcAlpha;
    blendTarget.DestBlend = BlendFactor::InvSrcAlpha;
    blendTarget.SrcBlendAlpha = BlendFactor::InvSrcAlpha;
    blendTarget.DestBlendAlpha = BlendFactor::Zero;

    psoDesc.RasterRenderState.CullMode = RasterCullMode::None;
    psoDesc.RasterRenderState.ScissorEnable = true;
    psoDesc.RasterRenderState.DepthClipEnable = true;

    psoDesc.DepthStencilRenderState.DepthTestEnable = false;
    psoDesc.DepthStencilRenderState.StencilEnable = false;

    // Build Root Signature
    Dx12::RootSignatureBuilder builder = {};

    builder.Add32BitConstants<999, 0>(sizeof(Shader::ImguiDrawInfo) / 4);
    builder.AddStaticSampler<0, 0>(
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0u,
        D3D12_COMPARISON_FUNC_ALWAYS,
        D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK);

    Dx12::DescriptorTable bindlessTable = {};
    Renderer::FillBindlessDescriptorTable(this->m_graphicsDevice, bindlessTable);

    builder.AddDescriptorTable(bindlessTable);

    psoDesc.RootSignatureBuilder = &builder;

    this->m_pso = this->m_graphicsDevice->CreateGraphicsPSOHandle(psoDesc);
}
