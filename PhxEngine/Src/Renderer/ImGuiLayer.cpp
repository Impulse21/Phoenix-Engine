#include <PhxEngine/Renderer/ImGuiLayer.h>

#include <PhxEngine/Core/Span.h>
#include <DirectXMath.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <GLFW/glfw3.h>

#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/ImguiVS.h"
#include "ImGui/ImguiPS.h"


namespace
{
	enum RootParameters
	{
		PushConstant,           // cbuffer vertexBuffer : register(b0)
		BindlessResources,
		NumRootParameters
	};
}

void PhxEngine::ImGuiLayer::OnAttach()
{
	PHX_EVENT();

	this->m_imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(this->m_imguiContext);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.FontAllowUserScaling = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	this->SetDarkThemeColors();

	Application& app = Application::GetInstance();
	GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
	this->m_resizeEventHandler = [this](WindowResizeEvent const& e) { Application::GetInstance().GetGfxDevice().DeleteGfxPipeline(this->m_pipeline); };

	EventBus::Subscribe<WindowResizeEvent>(this->m_resizeEventHandler);

	// Setup Platform/Renderer bindings
	if (!ImGui_ImplGlfw_InitForVulkan(glfwWindow, true))
	{
		throw std::runtime_error("Failed it initalize IMGUI");
		return;
	}


	unsigned char* pixelData = nullptr;
	int width;
	int height;

	io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

	// Create texture
	RHI::TextureDesc desc = {
		.Dimension = RHI::TextureDimension::Texture2D,
		.Format = RHI::Format::RGBA8_UNORM,
		.Width = (uint32_t)width,
		.Height = (uint32_t)height,
		.MipLevels = 1,
		.DebugName = "IMGUI Font Texture",
	};

	RHI::GfxDevice& gfxDevice = app.GetGfxDevice();
	this->m_fontTexture = gfxDevice.CreateTexture(desc);
	io.Fonts->SetTexID(static_cast<void*>(&this->m_fontTexture));

	RHI::SubresourceData subResourceData = {};
	subResourceData.rowPitch = width * 4;
	subResourceData.slicePitch = subResourceData.rowPitch * height;
	subResourceData.pData = pixelData;

	RHI::CommandListHandle uploadCommandList = gfxDevice.BeginCommandList(RHI::CommandQueueType::Copy);
	gfxDevice.TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest, uploadCommandList);
	gfxDevice.WriteTexture(this->m_fontTexture, 0, 1, &subResourceData, uploadCommandList);
	gfxDevice.TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource, uploadCommandList);

	this->CreatePipelineStateResources(gfxDevice);
}

void PhxEngine::ImGuiLayer::OnDetach()
{
	EventBus::Unsubscribe<WindowResizeEvent>(this->m_resizeEventHandler);

	Application& app = Application::GetInstance();
	app.GetGfxDevice().DeleteGfxPipeline(this->m_pipeline);
	app.GetGfxDevice().DeleteTexture(this->m_fontTexture);

	if (this->m_imguiContext)
	{
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext(this->m_imguiContext);
		this->m_imguiContext = nullptr;
	}
}

void PhxEngine::ImGuiLayer::Begin()
{
	ImGui::SetCurrentContext(this->m_imguiContext);
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void PhxEngine::ImGuiLayer::End()
{
	PHX_EVENT();
	using namespace RHI;

	Application& app = Application::GetInstance();
	GfxDevice& gfxDevice = app.GetGfxDevice();

	if (!this->m_pipeline.IsValid())
	{
		GfxPipelineDesc psoDesc = {
			.InputLayout = this->m_inputLayout,
			.VertexShader = this->m_vertexShader,
			.PixelShader = this->m_pixelShader,
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
			.RtvFormats = { gfxDevice.GetTextureDesc(gfxDevice.GetBackBuffer()).Format },
		};

		this->m_pipeline = gfxDevice.CreateGfxPipeline(psoDesc);
	}


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

	RHI::CommandListHandle cmd = gfxDevice.BeginCommandList();
	{
		RHI::ScopedMarker marker("ImGui", cmd);
		gfxDevice.SetGfxPipeline(this->m_pipeline, cmd);

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
		gfxDevice.SetViewports(&v, 1, cmd);

		gfxDevice.SetRenderTargets({ gfxDevice.GetBackBuffer() }, {}, cmd);
		const RHI::Format indexFormat = sizeof(ImDrawIdx) == 2 ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;

		for (int i = 0; i < drawData->CmdListsCount; ++i)
		{
			const ImDrawList* drawList = drawData->CmdLists[i];

			gfxDevice.BindDynamicVertexBuffer(0, drawList->VtxBuffer.size(), sizeof(ImDrawVert), drawList->VtxBuffer.Data, cmd);
			gfxDevice.BindDynamicIndexBuffer(drawList->IdxBuffer.size(), indexFormat, drawList->IdxBuffer.Data, cmd);

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
							? gfxDevice.GetDescriptorIndex(*textureHandle, RHI::SubresouceType::SRV)
							: RHI::cInvalidDescriptorIndex;

						gfxDevice.BindPushConstant(RootParameters::PushConstant, push, cmd);
						gfxDevice.SetScissors(&scissorRect, 1, cmd);
						gfxDevice.DrawIndexed({
								.IndexCount = drawCmd.ElemCount,
								.InstanceCount = 1,
								.StartIndex = static_cast<uint32_t>(indexOffset),
							},
							cmd);
					}
				}
				indexOffset += drawCmd.ElemCount;
			}
		}

		// cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
		gfxDevice.EndMarker(cmd);
		ImGui::EndFrame();
	}
}


uint32_t PhxEngine::ImGuiLayer::GetActiveWidgetID() const
{
    return 0;
}

void PhxEngine::ImGuiLayer::CreatePipelineStateResources(RHI::GfxDevice& gfxDevice)
{
	using namespace RHI;

	
	this->m_vertexShader = gfxDevice.CreateShader({
			.Stage = RHI::ShaderStage::Vertex,
			.DebugName = "ImGuiVS",
		},
		PhxEngine::Span<uint8_t>(static_cast<const uint8_t*>(g_mainVS), sizeof(g_mainVS) / sizeof(unsigned char)));

	this->m_pixelShader = gfxDevice.CreateShader(
		{
			.Stage = RHI::ShaderStage::Pixel,
			.DebugName = "ImGuiPS",
		},
		PhxEngine::Span<uint8_t>(static_cast<const uint8_t*>(g_mainPS), sizeof(g_mainPS) / sizeof(unsigned char)));

	std::vector<VertexAttributeDesc> attributeDesc =
	{
		{ "POSITION",   0, RHI::Format::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
		{ "TEXCOORD",   0, RHI::Format::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
		{ "COLOR",      0, RHI::Format::RGBA8_UNORM, 0, VertexAttributeDesc::SAppendAlignedElement, false},
	};

	this->m_inputLayout = gfxDevice.CreateInputLayout(attributeDesc.data(), attributeDesc.size());
}

void PhxEngine::ImGuiLayer::SetDarkThemeColors()
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
