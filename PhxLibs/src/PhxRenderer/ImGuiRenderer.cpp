#include "PhxRenderer/PhxRenderer_pch.h"

#include "PhxRenderer/ImGuiRenderer.h"

#include <DirectXMath.h>

#include "PhxRhi/RHICore.h"
#include <PhxRenderer/shaders/PrecompiledShaders.h>

#include "ImGui/imgui_impl_win32.h"
#include "PhxCore/Span.h"
#include "PhxCore/VFS.h"

using namespace phx;
using namespace phx::gfx;
using namespace phx::rhi;

namespace
{
    enum RootParameters
    {
        PushConstant,           // cbuffer vertexBuffer : register(b0)
        BindlessResources,
        NumRootParameters
    };
}

void ImGuiRenderSystem::Initialize(IFileSystem* /*fs*/, void* windowHandle, bool enableDocking)
{
    m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imguiContext);

    if (!ImGui_ImplWin32_Init(windowHandle))
    {
        throw std::runtime_error("Failed it initalize IMGUI");
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

    MemInfo memInfo = {};
    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    memInfo.RowPitch = width * 4;
    memInfo.SlicePitch = memInfo.RowPitch * height;
    memInfo.Data = pixelData;

    // Create texture
    m_fontTexture = rhi::CreateTexture({
            .DebugName = "ImGui Font",
            .Format = rhi::Format::RGBA8_UNORM,
            .Width = static_cast<uint32_t>(width),
            .Height = static_cast<uint32_t>(height),
            .ArraySize = 1,
        }, &memInfo);
    
    this->m_fontTextureBindlessIndex = rhi::GetDescriptorIndex(this->m_fontTexture, SubresouceType::SRV);
    io.Fonts->SetTexID(static_cast<ImTextureID>(this->m_fontTextureBindlessIndex));
    
    Span vsOutBytesCode = Span(ImGuiVS::g_MainVS, sizeof(ImGuiVS::g_MainVS));
    Span psOutBytesCode = Span(ImGuiPS::g_MainPS, sizeof(ImGuiPS::g_MainPS));

	m_pipeline = rhi::CreatePipelineState({
            .VS = {.ByteCode = vsOutBytesCode, .EntryPoint = "MainVS" },
            .PS = {.ByteCode = psOutBytesCode, .EntryPoint = "MainPS" },
		    .BlendState = {
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
		    .DepthStencilState = { .DepthEnable = false, .DepthWriteMask = DepthWriteMask::Zero },
		    .RasterState = { .CullMode = RasterCullMode::None, .DepthClipEnable = true, .ScissorEnable = true },
            .VertexBufferBindings = {
                {
                    .SemanticName = "POSITION",
                    .Format = Format::RG32_FLOAT,
                },
                {
                    .SemanticName = "TEXCOORD",
                    .Format = Format::RG32_FLOAT,
                },
                {
                    .SemanticName = "COLOR",
                    .Format = Format::RGBA8_UNORM,
                },
            },
            .RenderPassInfo = {.RTFormats { rhi::Format::UNKNOWN }}
        });
}

void ImGuiRenderSystem::Finialize()
{
    rhi::DeleteTexture(m_fontTexture);
    rhi::DeletePipeline(m_pipeline);
}

void ImGuiRenderSystem::EnableDarkThemeColours()
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
    colors[ImGuiCol_TabSelected] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabDimmed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabDimmedSelected] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

void ImGuiRenderSystem::BeginFrame()
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderSystem::Render(CommandCtx* ctx)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();

    // Check if there is anything to render.
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return;
    }

    ImVec2 displayPos = drawData->DisplayPos;

    {
        ctx->SetPipelineState(m_pipeline);

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
        ctx->SetViewports({ v });

        const Format indexFormat = sizeof(ImDrawIdx) == 2 ? Format::R16_UINT : Format::R32_UINT;

        for (int i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];
            ctx->SetDynamicVertexBuffer(0, drawList->VtxBuffer.size(), sizeof(ImDrawVert), drawList->VtxBuffer.Data);
            ctx->SetDynamicIndexBuffer(drawList->IdxBuffer.size(), indexFormat, drawList->IdxBuffer.Data);

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

                    if (scissorRect.MaxX - scissorRect.MinX > 0 &&
                        scissorRect.MaxY - scissorRect.MinY > 0)
                    {
                        auto desciptorIndex = static_cast<DescriptorIndex>(drawCmd.GetTexID());
                        push.TextureIndex = desciptorIndex;
                        ctx->SetPushConstant(RootParameters::PushConstant, sizeof(ImguiDrawInfo), &push);
                        ctx->SetScissors({ &scissorRect, 1 });
                        ctx->DrawIndexed(drawCmd.ElemCount, 1, indexOffset, 0, 0);
                    }
                }
                indexOffset += drawCmd.ElemCount;
            }
        }

        // cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
        ImGui::EndFrame();
    }
}
