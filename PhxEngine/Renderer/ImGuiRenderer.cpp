#include <Renderer/ImGuiRenderer.h>

#include <RHI/PhxRHI.h>

#include <Core/Window.h>
#include <Core/Span.h>
#include <stdexcept>
#include <DirectXMath.h>
#include <imgui.h>
#include "ImGui/imgui_impl_glfw.h"

#include "ImGui/ImguiVS.h"
#include "ImGui/ImguiPS.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Renderer;



#if 0
namespace
{
    enum RootParameters
    {
        PushConstant,           // cbuffer vertexBuffer : register(b0)
        BindlessResources,
        NumRootParameters
    };

    ImGuiContext* m_imguiContext;

    RHI::TextureHandle m_fontTexture;
    RHI::ShaderHandle m_vertexShader;
    RHI::ShaderHandle m_pixelShader;
    RHI::InputLayoutHandle m_inputLayout;
    RHI::GfxPipelineHandle m_pipeline;
    RHI::SwapChainHandle m_swapchainHandle;
}
void PhxEngine::Renderer::ImGuiRenderer::Initialize(PhxEngine::Core::IWindow* window, RHI::SwapChainHandle handle, bool enableDocking)
{
    m_swapchainHandle = handle;
    m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imguiContext);
    auto* glfwWindow = static_cast<GLFWwindow*>(window->GetInternalWindow());

    if (!ImGui_ImplGlfw_InitForVulkan(glfwWindow, true))
    {
        throw std::runtime_error("Failed it initalize IMGUI");
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontAllowUserScaling = true;
    // Drive this based on configuration
    if (enableDocking)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    unsigned char* pixelData = nullptr;
    int width;
    int height;

    io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

    // Create texture
    RHI::TextureDesc desc = {};
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.Width = width;
    desc.Height = height;
    desc.Format = RHI::Format::RGBA8_UNORM;
    desc.MipLevels = 1;
    desc.DebugName = "IMGUI Font Texture";

    RHI::SubresourceData subResourceData = {};
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    m_fontTexture = RHI::CreateTexture(desc, &subResourceData);
    io.Fonts->SetTexID(static_cast<void*>(&m_fontTexture));
    
    m_vertexShader = RHI::CreateShader( {
            .Stage = RHI::ShaderStage::Vertex,
            .DebugName = "ImGuiVS",
        },
        PhxEngine::Core::Span<uint8_t>(static_cast<const uint8_t*>(g_mainVS), sizeof(g_mainVS) / sizeof(unsigned char)));

    m_pixelShader = RHI::CreateShader(
        {
            .Stage = RHI::ShaderStage::Pixel,
            .DebugName = "ImGuiPS",
        },
        PhxEngine::Core::Span<uint8_t>(static_cast<const uint8_t*>(g_mainPS), sizeof(g_mainPS) / sizeof(unsigned char)));


    std::vector<VertexAttributeDesc> attributeDesc =
    {
        { "POSITION",   0, RHI::Format::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "TEXCOORD",   0, RHI::Format::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "COLOR",      0, RHI::Format::RGBA8_UNORM, 0, VertexAttributeDesc::SAppendAlignedElement, false},
    };

    m_inputLayout = RHI::CreateInputLayout(attributeDesc);
}

void PhxEngine::Renderer::ImGuiRenderer::Finalize()
{
    // RHI::DeleteGfxPipeline(m_pipeline);
    // cmd->DeleteInputLayout(m_inputLayout);
    // cmd->DeleteShader(m_pixelShader);
    // cmd->DeleteShader(m_vertexShader);
    // RHI::DeleteTexture(m_fontTexture);

    ImGui::DestroyContext(m_imguiContext);
}

void PhxEngine::Renderer::ImGuiRenderer::BeginFrame()
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void PhxEngine::Renderer::ImGuiRenderer::Render(RHI::CommandList* cmd)
{
    if (!m_pipeline.IsValid())
    {
        GfxPipelineDesc psoDesc = {
            .InputLayout = m_inputLayout,
            .VertexShader = m_vertexShader,
            .PixelShader = m_pixelShader,
            .BlendRenderState = {
                .Targets {
                    {
                        .BlendEnable = true,
                        .SrcBlend = BlendFactor::SrcAlpha,
                        .DestBlend = BlendFactor::InvSrcAlpha,
                        .BlendOp = EBlendOp::Add,
                        .SrcBlendAlpha = BlendFactor::One,
                        .DestBlendAlpha = BlendFactor::InvSrcAlpha,
                        .BlendOpAlpha = EBlendOp::Add,
                        .ColorWriteMask = ColorMask::All,
                    }
                }
            },
            .DepthStencilRenderState = {
                .DepthTestEnable = false,
                .DepthFunc = ComparisonFunc::Always,
                .StencilEnable = false,
            },
            .RasterRenderState = {
                .CullMode = RasterCullMode::None,
                .DepthClipEnable = true,
                .ScissorEnable = true,
            },
            .RtvFormats = {  RHI::GetSwapChainFormat(m_swapchainHandle) },
        };

        m_pipeline = RHI::CreateGfxPipeline(psoDesc);
    }

    ImGui::SetCurrentContext(m_imguiContext);
    ImGui::Render();


    ImGuiIO& io = ImGui::GetIO();
    ImDrawData* drawData = ImGui::GetDrawData();

    // Check if there is anything to render.
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return;
    }

    ImVec2 displayPos = drawData->DisplayPos;
    {
        cmd->BeginMarker("ImGui");
        cmd->SetGfxPipeline(m_pipeline);

        // Set root arguments.
        //    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixOrthographicRH( drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 1.0f );
        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        static const float mvp[4][4] =
        {
            { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
        };

        struct ImguiDrawInfo
        {
            DirectX::XMFLOAT4X4 Mvp;
            uint32_t TextureIndex;
        } push = {};
        push.Mvp = DirectX::XMFLOAT4X4(&mvp[0][0]);

        Viewport v(drawData->DisplaySize.x, drawData->DisplaySize.y);
        cmd->SetViewports(&v, 1);
        
        cmd->SetRenderTargets({ m_swapchainHandle });
        const RHI::Format indexFormat = sizeof(ImDrawIdx) == 2 ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;

        for (int i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];

            cmd->BindDynamicVertexBuffer(0, drawList->VtxBuffer.size(), sizeof(ImDrawVert), drawList->VtxBuffer.Data);
            cmd->BindDynamicIndexBuffer(drawList->IdxBuffer.size(), indexFormat, drawList->IdxBuffer.Data);

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
                        // Ensure 
                        auto textureHandle = static_cast<RHI::TextureHandle*>(drawCmd.GetTexID());
                        push.TextureIndex = textureHandle
                            ? RHI::GetDescriptorIndex(*textureHandle, RHI::SubresouceType::SRV)
                            : RHI::cInvalidDescriptorIndex;

                        cmd->BindPushConstant(RootParameters::PushConstant, push);
                        cmd->SetScissors(&scissorRect, 1);
                        cmd->DrawIndexed({
                                .IndexCount = drawCmd.ElemCount,
                                .InstanceCount = 1,
                                .StartIndex = static_cast<uint32_t>(indexOffset),
                            });
                    }
                }
                indexOffset += drawCmd.ElemCount;
            }
        }

        // cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
        cmd->EndMarker();
        ImGui::EndFrame();
    }
}

void PhxEngine::Renderer::ImGuiRenderer::EnableDarkThemeColours()
{

    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

    // Headers
    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}
#else

void PhxEngine::Renderer::ImGuiRenderer::Initialize(PhxEngine::Core::IWindow* window, RHI::SwapChain const& handle, bool enableDocking)
{
}

void PhxEngine::Renderer::ImGuiRenderer::Finalize()
{
}

void PhxEngine::Renderer::ImGuiRenderer::BeginFrame()
{
}

void PhxEngine::Renderer::ImGuiRenderer::Render(RHI::CommandList* cmd)
{
}

void PhxEngine::Renderer::ImGuiRenderer::EnableDarkThemeColours()
{
}

#endif