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

    m_inputLayout = GfxDevice::CreateInputLayout(attributeDesc);

    m_pipeline = GfxDevice::CreateGfxPipeline({
        .InputLayout = m_inputLayout,
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
    SubresourceData subResourceData = {};

    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    CommandListHandle uploadCommandList = gfxDevice->BeginCommandList();

    gfxDevice->TransitionBarrier(m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest, uploadCommandList);
    gfxDevice->WriteTexture(m_fontTexture, 0, 1, &subResourceData, uploadCommandList);
    gfxDevice->TransitionBarrier(m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource, uploadCommandList);


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
