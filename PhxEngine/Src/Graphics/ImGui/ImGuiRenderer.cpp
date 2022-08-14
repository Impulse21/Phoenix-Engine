#include "phxpch.h"

#include "Graphics/ImGui/ImGuiRenderer.h"
#include <PhxEngine/Graphics/ShaderStore.h>
#include <Shaders/ShaderInteropStructures.h>
#include <PhxEngine/App/Application.h>
#include <PhxEngine/Core/Window.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include <unordered_set>

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

    IWindow* window = LayeredApplication::Ptr->GetWindow();
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
    desc.Format = RHI::FormatType::RGBA8_UNORM;
    desc.MipLevels = 1;
    desc.DebugName = "IMGUI Font Texture";

    this->m_fontTexture = IGraphicsDevice::Ptr->CreateTexture(desc);

    RHI::SubresourceData subResourceData = {};

    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    CommandListHandle uploadCommandList = IGraphicsDevice::Ptr->CreateCommandList();

    uploadCommandList->Open();
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    uploadCommandList->WriteTexture(this->m_fontTexture, 0, 1, &subResourceData);
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);

    uploadCommandList->Close();

    IGraphicsDevice::Ptr->ExecuteCommandLists(uploadCommandList.get(), true);
    this->CreatePipelineStateObject(IGraphicsDevice::Ptr);
}

void PhxEngine::Graphics::ImGuiRenderer::OnDetach()
{
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
	cmd->SetGraphicsPSO(this->m_pso);
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

	const FormatType indexFormat = sizeof(ImDrawIdx) == 2 ? FormatType::R16_UINT : FormatType::R32_UINT;

    /*
    static thread_local std::unordered_set<RHI::TextureHandle> cache;
    static thread_local std::vector<RHI::GpuBarrier> preBarriers;
    static thread_local std::vector<RHI::GpuBarrier> postBarriers;
    cache.clear();
    preBarriers.clear();
    postBarriers.clear();

    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const ImDrawList* drawList = drawData->CmdLists[i];
        for (int j = 0; j < drawList->CmdBuffer.size(); ++j)
        {
            const ImDrawCmd& drawCmd = drawList->CmdBuffer[j];

            if (drawCmd.UserCallback)
            {
                continue;
            }
            // Ensure 
            /*PhxEngine::RHI::TextureHandle texture = *reinterpret_cast<PhxEngine::RHI::TextureHandle*>(drawCmd.TextureId);

            if (texture && cache.find(texture) == cache.end())
            {
                cache.insert(texture);
                preBarriers.push_back(GpuBarrier::CreateTexture(texture, texture->GetDesc().InitialState, RHI::ResourceStates::ShaderResource));
                postBarriers.push_back(GpuBarrier::CreateTexture(texture, RHI::ResourceStates::ShaderResource, texture->GetDesc().InitialState));
            }
        }
    }

    cmd->TransitionBarriers(Span<GpuBarrier>(preBarriers.data(), preBarriers.size()));

    */

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
                    auto textureIndex = reinterpret_cast<RHI::DescriptorIndex*>(drawCmd.TextureId);

                    push.TextureIndex = textureIndex && *textureIndex != RHI::cInvalidDescriptorIndex
                        ? *textureIndex
                        : IGraphicsDevice::Ptr->GetDescriptorIndex(m_fontTexture);

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
    ShaderHandle vs = ShaderStore::Ptr->Retrieve(PreLoadShaders::VS_ImGui);
    ShaderHandle ps = ShaderStore::Ptr->Retrieve(PreLoadShaders::PS_ImGui);

    std::vector<VertexAttributeDesc> attributeDesc =
    {
        { "POSITION",   0, FormatType::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "TEXCOORD",   0, FormatType::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        { "COLOR",      0, FormatType::RGBA8_UNORM, 0, VertexAttributeDesc::SAppendAlignedElement, false},
    };

    InputLayoutHandle inputLayout = graphicsDevice->CreateInputLayout(attributeDesc.data(), attributeDesc.size());

    GraphicsPSODesc psoDesc = {};
    psoDesc.VertexShader = vs;
    psoDesc.PixelShader = ps;
    psoDesc.InputLayout = inputLayout;
    // TODO: Render to it's own resource, then compose image.
    // TODO: Set it's own Format
    psoDesc.RtvFormats.push_back(FormatType::R10G10B10A2_UNORM);

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
    */

    this->m_pso = graphicsDevice->CreateGraphicsPSO(psoDesc);
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
