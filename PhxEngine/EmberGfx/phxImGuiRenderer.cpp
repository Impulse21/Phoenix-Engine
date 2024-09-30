#include "pch.h"
#include "phxImGuiRenderer.h"

#include "ImGui/imgui_impl_win32.h"
#include "phxGfxDevice.h"
#include "phxSpan.h"

#include "ImGui/ImguiVS.h"
#include "ImGui/ImguiPS.h"
namespace phx
{
    namespace phx::EngineCore { extern HWND g_hWnd; }

    extern gfx::Format g_SwapChainFormat;
}

using namespace phx::gfx;
namespace
{
}


void phx::gfx::ImGuiRenderSystem::Initialize(bool enableDocking)
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

    // Create texture
    m_fontTexture = GfxDevice::CreateTexture({
        .Format = gfx::Format::RGBA8_UNORM,
        .Width = static_cast<uint32_t>(width),
        .Height = static_cast<uint32_t>(height),
        .DebugName = "ImGui Font"
        });

    io.Fonts->SetTexID(static_cast<void*>(&m_fontTexture));

    std::vector<VertexAttributeDesc> attributeDesc =
    {
        { "POSITION",   0, Format::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "TEXCOORD",   0, Format::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "COLOR",      0, Format::RGBA8_UNORM, 0, VertexAttributeDesc::SAppendAlignedElement, false},
    };

    m_pipeline = GfxDevice::CreateGfxPipeline({
        .InputLayout = GfxDevice::CreateInputLayout(attributeDesc),
        .VertexShaderByteCode = Span(g_mainVS, ARRAYSIZE(g_mainVS)),
        .HullShaderByteCode = Span(g_mainPS, ARRAYSIZE(g_mainPS)),
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
        .RtvFormats = { g_SwapChainFormat }
     });

#if false


    m_vertexShader = gfxDevice->CreateShader({
            .Stage = RHI::ShaderStage::Vertex,
            .DebugName = "ImGuiVS",
        },
        PhxEngine::Core::Span<uint8_t>(static_cast<const uint8_t*>(g_mainVS), sizeof(g_mainVS) / sizeof(unsigned char)));

    m_pixelShader = gfxDevice->CreateShader(
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

    m_inputLayout = gfxDevice->CreateInputLayout(attributeDesc.data(), attributeDesc.size());
#endif
}

void phx::gfx::ImGuiRenderSystem::Render(CommandCtx& context)
{
    if (!this->m_isFontTextureUploaded)
    {
        unsigned char* pixelData = nullptr;
        int width;
        int height;

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

        SubresourceData subResourceData = {};
        // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
        subResourceData.rowPitch = width * 4;
        subResourceData.slicePitch = subResourceData.rowPitch * height;
        subResourceData.pData = pixelData;

        GpuBarrier barrier = GpuBarrier::CreateTexture(this->m_fontTexture, gfx::ResourceStates::Common, gfx::ResourceStates::CopyDest);
        context.TransitionBarrier(barrier);

        context.WriteTexture(this->m_fontTexture, 0, 1, &subResourceData);

        barrier = GpuBarrier::CreateTexture(this->m_fontTexture, gfx::ResourceStates::Common, gfx::ResourceStates::CopyDest);
        context.TransitionBarrier(barrier);
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
        context.SetGfxPipeline(m_pipeline);

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
        context.SetViewports({ v });

        context.SetRenderTargetSwapChain();
        const Format indexFormat = sizeof(ImDrawIdx) == 2 ? Format::R16_UINT : Format::R32_UINT;

        for (int i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];

            TempBuffer vertexBuffer = GfxDevice::AllocateTemp(drawList->VtxBuffer.size() * sizeof(ImDrawVert));
            std::memcpy(vertexBuffer.Data, drawList->VtxBuffer.Data, sizeof(ImDrawVert));
            context.SetDynamicVertexBuffer(vertexBuffer.Buffer, vertexBuffer.Offset, 0, drawList->VtxBuffer.size(), sizeof(ImDrawVert));

            TempBuffer indexBuffer = GfxDevice::AllocateTemp(drawList->IdxBuffer.size() * sizeof(ImDrawIdx));
            std::memcpy(indexBuffer.Data, drawList->IdxBuffer.Data, drawList->IdxBuffer.size() * sizeof(ImDrawVert));
            context.SetDynamicIndexBuffer(indexBuffer.Buffer, indexBuffer.Offset, drawList->IdxBuffer.size(), indexFormat);

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
#if false
                        auto texture = static_cast<TextureRef>(drawCmd.GetTexID());
                        push.TextureIndex = texture
                            ? m_gfxDevice->GetDescriptorIndex(*textureHandle, RHI::SubresouceType::SRV)
                            : RHI::cInvalidDescriptorIndex;
                        m_gfxDevice->BindPushConstant(RootParameters::PushConstant, push, cmd);
                        m_gfxDevice->SetScissors(&scissorRect, 1, cmd);
                        m_gfxDevice->DrawIndexed({
                                .IndexCount = drawCmd.ElemCount,
                                .InstanceCount = 1,
                                .StartIndex = static_cast<uint32_t>(indexOffset),
                            },
                            cmd);
#endif

                    }
                }
                indexOffset += drawCmd.ElemCount;
            }
        }

        // cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
        ImGui::EndFrame();
    }
}
