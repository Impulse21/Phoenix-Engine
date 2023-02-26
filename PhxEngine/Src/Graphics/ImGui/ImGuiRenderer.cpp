#include "phxpch.h"

#include "Graphics/ImGui/ImGuiRenderer.h"
#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Engine/EngineApp.h>
#include <PhxEngine/Core/Window.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include <unordered_set>
#include "ImGuiRenderer.h"

using namespace PhxEngine::Graphics;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

enum RootParameters
{
    PushConstant,           // cbuffer vertexBuffer : register(b0)
    BindlessResources,
    NumRootParameters
};

void PhxEngine::Graphics::ImGuiRenderer::OnAttach()
{
    this->m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(this->m_imguiContext);

    IWindow* window = EngineApp::GPtr->GetWindow();
    auto* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());

    if (!ImGui_ImplGlfw_InitForVulkan(glfwWindow, true))
    {
        throw std::runtime_error("Failed it initalize IMGUI");
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontAllowUserScaling = true;
    // Drive this based on configuration
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();
    SetDarkThemeColors();

    unsigned char* pixelData = nullptr;
    int width;
    int height;

    io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

    // Create texture
    RHI::TextureDesc desc = {};
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.Width = width;
    desc.Height = height;
    desc.Format = RHI::RHIFormat::RGBA8_UNORM;
    desc.MipLevels = 1;
    desc.DebugName = "IMGUI Font Texture";

    this->m_fontTexture = IGraphicsDevice::GPtr->CreateTexture(desc);
    io.Fonts->SetTexID(static_cast<void*>(&this->m_fontTexture));
    RHI::SubresourceData subResourceData = {};

    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    CommandListHandle uploadCommandList = IGraphicsDevice::GPtr->CreateCommandList();

    uploadCommandList->Open();
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    uploadCommandList->WriteTexture(this->m_fontTexture, 0, 1, &subResourceData);
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);

    uploadCommandList->Close();

    IGraphicsDevice::GPtr->ExecuteCommandLists({ uploadCommandList.get() }, true);
    this->CreatePipelineStateObject(IGraphicsDevice::GPtr);
}

void PhxEngine::Graphics::ImGuiRenderer::OnDetach()
{
    IGraphicsDevice::GPtr->DeleteTexture(this->m_fontTexture);

    if (this->m_imguiContext)
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(this->m_imguiContext);
        this->m_imguiContext = nullptr;
    }
}

void PhxEngine::Graphics::ImGuiRenderer::BeginFrame()
{
    ImGui::SetCurrentContext(this->m_imguiContext);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void PhxEngine::Graphics::ImGuiRenderer::OnCompose(RHI::CommandListHandle cmd)
{
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

	auto scrope = cmd->BeginScopedMarker("ImGui");
	cmd->SetGraphicsPipeline(this->m_pso);
	cmd->BindResourceTable(RootParameters::BindlessResources);

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

	Shader::ImguiDrawInfo push = {};
	push.Mvp = DirectX::XMFLOAT4X4(&mvp[0][0]);

	Viewport v(drawData->DisplaySize.x, drawData->DisplaySize.y);
	cmd->SetViewports(&v, 1);

	const RHIFormat indexFormat = sizeof(ImDrawIdx) == 2 ? RHIFormat::R16_UINT : RHIFormat::R32_UINT;

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
                        ? IGraphicsDevice::GPtr->GetDescriptorIndex(*textureHandle, RHI::SubresouceType::SRV)
                        : RHI::cInvalidDescriptorIndex;

                    cmd->BindPushConstant(RootParameters::PushConstant, push);
					cmd->SetScissors(&scissorRect, 1);
					cmd->DrawIndexed(drawCmd.ElemCount, 1, indexOffset);
				}
			}
			indexOffset += drawCmd.ElemCount;
		}
	}

    // cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
	ImGui::EndFrame();
}

void PhxEngine::Graphics::ImGuiRenderer::CreatePipelineStateObject(
    RHI::IGraphicsDevice* graphicsDevice)
{
    ShaderHandle vs = ShaderStore::GPtr->Retrieve(PreLoadShaders::VS_ImGui);
    ShaderHandle ps = ShaderStore::GPtr->Retrieve(PreLoadShaders::PS_ImGui);

    std::vector<VertexAttributeDesc> attributeDesc =
    {
        { "POSITION",   0, RHIFormat::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "TEXCOORD",   0, RHIFormat::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "COLOR",      0, RHIFormat::RGBA8_UNORM, 0, VertexAttributeDesc::SAppendAlignedElement, false},
    };

    InputLayoutHandle inputLayout = graphicsDevice->CreateInputLayout(attributeDesc.data(), attributeDesc.size());

    GraphicsPipelineDesc psoDesc = {};
    psoDesc.VertexShader = vs;
    psoDesc.PixelShader = ps;
    psoDesc.InputLayout = inputLayout;
    // TODO: Render to it's own resource, then compose image.
    // TODO: Set it's own Format
    psoDesc.RtvFormats = { graphicsDevice->GetTextureDesc(graphicsDevice->GetBackBuffer()).Format };

    auto& blendTarget = psoDesc.BlendRenderState.Targets[0];
    blendTarget.BlendEnable = true;
    blendTarget.SrcBlend = BlendFactor::SrcAlpha;
    blendTarget.DestBlend = BlendFactor::InvSrcAlpha;
    blendTarget.BlendOp = EBlendOp::Add;
    blendTarget.SrcBlendAlpha = BlendFactor::One;
    blendTarget.DestBlendAlpha = BlendFactor::InvSrcAlpha;
    blendTarget.BlendOpAlpha = EBlendOp::Add;
    blendTarget.ColorWriteMask = ColorMask::All;

    psoDesc.RasterRenderState.CullMode = RasterCullMode::None;
    psoDesc.RasterRenderState.ScissorEnable = true;
    psoDesc.RasterRenderState.DepthClipEnable = true;

    psoDesc.DepthStencilRenderState.DepthTestEnable = false;
    psoDesc.DepthStencilRenderState.StencilEnable = false;
    psoDesc.DepthStencilRenderState.DepthFunc = ComparisonFunc::Always;

    // TODO: Build Root Signature
    /*
    Dx12::RootSignatureBuilder builder = {};

    builder.Add32BitConstants<999, 0>(sizeof(Shader::ImguiDrawInfo) / 4);
    builder.AddStaticSampler<0, 0>(
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0u,
        D3D12_COMPARISON_FUNC_ALWAYS,
        D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK);
       
    Dx12::DescriptorTable bindlessTable = {};
    Renderer::FillBindlessDescriptorTable(graphicsDevice, bindlessTable);

    builder.AddDescriptorTable(bindlessTable);

    psoDesc.RootSignatureBuilder = &builder;

    this->m_pso = graphicsDevice->CreateGraphicsP00
        ipeline(psoDesc);
    */
}

void PhxEngine::Graphics::ImGuiRenderer::SetDarkThemeColors()
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
