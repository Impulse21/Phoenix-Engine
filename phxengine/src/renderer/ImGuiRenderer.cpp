#include "pch.h"
#include "phx/renderer/ImGuiRenderer.h"

#include "ImGui/imgui_impl_win32.h"
#include "phx/core/Span.h"
#include "phx/core/VFS.h"

#include "phx/rhi/ShaderCompiler.h"
#include "phx/rhi/CommandListRecorder.h"

namespace phx
{
    namespace EngineCore { extern HWND g_hWnd; }
}

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

void ImGuiRenderSystem::Initialize(GfxDevice* gfxDevice, IFileSystem* fs, bool enableDocking)
{
    m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imguiContext);

    if (!ImGui_ImplWin32_Init(phx::EngineCore::g_hWnd))
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

    MemInfo memInfo = {};
    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    memInfo.RowPitch = width * 4;
    memInfo.SlicePitch = memInfo.RowPitch * height;
    memInfo.Data = pixelData;

    // Create texture
    m_fontTexture = gfxDevice->CreateTexture({
        .DebugName = "ImGui Font",
        .Format = rhi::Format::RGBA8_UNORM,
        .Width = static_cast<uint32_t>(width),
        .Height = static_cast<uint32_t>(height)
        }, &memInfo);
    
    this->m_fontTextureBindlessIndex = gfxDevice->GetDescriptorIndex(this->m_fontTexture, SubresouceType::SRV);
    io.Fonts->SetTexID(static_cast<void*>(&this->m_fontTextureBindlessIndex));
    
    ShaderCompiler::Output vsOut = ShaderCompiler::Compile({
            .Format = gfxDevice->GetShaderFormat(),
            .ShaderStage = ShaderStage::VS,
            .SourceFilename = "/shaders_engine/ImGui.hlsl",
            .EntryPoint = "MainVS",
            .FileSystem = fs });

    ShaderCompiler::Output psOut = ShaderCompiler::Compile({
            .Format = gfxDevice->GetShaderFormat(),
            .ShaderStage = ShaderStage::PS,
            .SourceFilename = "/shaders_engine/ImGui.hlsl",
            .EntryPoint = "MainPS",
            .FileSystem = fs });

	m_pipeline = gfxDevice->CreatePipeline({
            .VS = {.ByteCode = vsOut.ByteCode, .EntryPoint = "MainVS" },
            .PS = {.ByteCode = psOut.ByteCode, .EntryPoint = "MainPS" },
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

void ImGuiRenderSystem::Finialize(GfxDevice* gfxDevice)
{
    gfxDevice->DeleteTexture(m_fontTexture);
    gfxDevice->DeletePipeline(m_pipeline);
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
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

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

void ImGuiRenderSystem::Render(GfxCommandListRecorder& recorder)
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
        recorder.SetPipelineState(m_pipeline);

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
        recorder.SetViewports({ v });

        const Format indexFormat = sizeof(ImDrawIdx) == 2 ? Format::R16_UINT : Format::R32_UINT;

        for (int i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];
            recorder.SetDynamicVertexBuffer(0, drawList->VtxBuffer.size(), sizeof(ImDrawVert), drawList->VtxBuffer.Data);
            recorder.SetDynamicIndexBuffer(drawList->IdxBuffer.size(), indexFormat, drawList->IdxBuffer.Data);

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
                        auto* desciptorIndex = static_cast<DescriptorIndex*>(drawCmd.GetTexID());
                        push.TextureIndex = desciptorIndex
                            ? *desciptorIndex
                            : cInvalidDescriptorIndex;
                        recorder.SetPushConstant(RootParameters::PushConstant, sizeof(ImguiDrawInfo), &push);
                        recorder.SetScissors({ &scissorRect, 1 });
                        recorder.DrawIndexed(drawCmd.ElemCount, 1, indexOffset, 0, 0);
                    }
                }
                indexOffset += drawCmd.ElemCount;
            }
        }

        // cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
        ImGui::EndFrame();
    }
    return;
}
