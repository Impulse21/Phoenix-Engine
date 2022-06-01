#include "phxpch.h"

#include "Graphics/ImGui/ImGuiRenderer.h"
#include "Graphics/ShaderStore.h"
#include <Shaders/ShaderInteropStructures.h>

#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_impl_win32.h"

using namespace PhxEngine::Graphics;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

enum RootParameters
{
    MatrixCB,           // cbuffer vertexBuffer : register(b0)
    BindlessResources,
    NumRootParameters
};

void PhxEngine::Graphics::ImGuiRenderer::Initialize(
    RHI::IGraphicsDevice* graphicsDevice,
    ShaderStore const& shaderStore,
    Core::Platform::WindowHandle windowHandle)
{
    this->m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(this->m_imguiContext);

    if (!ImGui_ImplWin32_Init(windowHandle))
    {
        throw std::runtime_error("Failed it initalize IMGUI");
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
    desc.Format = RHI::FormatType::RGBA8_UNORM;
    desc.MipLevels = 1;
    desc.DebugName = "IMGUI Font Texture";

    this->m_fontTexture = graphicsDevice->CreateTexture(desc);

    RHI::SubresourceData subResourceData = {};

    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    CommandListHandle uploadCommandList = graphicsDevice->CreateCommandList();

    uploadCommandList->Open();
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    uploadCommandList->WriteTexture(this->m_fontTexture, 0, 1, &subResourceData);
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);

    uploadCommandList->Close();

    graphicsDevice->ExecuteCommandLists(uploadCommandList.get(), true);
    this->CreatePipelineStateObject(graphicsDevice, shaderStore);
}

void PhxEngine::Graphics::ImGuiRenderer::Finalize()
{
    if (this->m_imguiContext)
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(this->m_imguiContext);
        this->m_imguiContext = nullptr;
    }
}

void PhxEngine::Graphics::ImGuiRenderer::BeginFrame()
{
    ImGui::SetCurrentContext(this->m_imguiContext);
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
}

void PhxEngine::Graphics::ImGuiRenderer::Render(RHI::CommandListHandle cmd)
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
	push.TextureIndex = this->m_fontTexture->GetDescriptorIndex();

	cmd->BindPushConstant(RootParameters::MatrixCB, push);
	Viewport v(drawData->DisplaySize.x, drawData->DisplaySize.y);
	cmd->SetViewports(&v, 1);

	const FormatType indexFormat = sizeof(ImDrawIdx) == 2 ? FormatType::R16_UINT : FormatType::R32_UINT;

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
					cmd->SetScissors(&scissorRect, 1);
					cmd->DrawIndexed(drawCmd.ElemCount, 1, indexOffset);
				}
			}
			indexOffset += drawCmd.ElemCount;
		}
	}

	ImGui::EndFrame();
}

void PhxEngine::Graphics::ImGuiRenderer::CreatePipelineStateObject(
    RHI::IGraphicsDevice* graphicsDevice,
    ShaderStore const& shaderStore)
{
    ShaderHandle vs = shaderStore.Retrieve(PreLoadShaders::VS_ImGui);
    ShaderHandle ps = shaderStore.Retrieve(PreLoadShaders::PS_ImGui);

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

    this->m_pso = graphicsDevice->CreateGraphicsPSOHandle(psoDesc);
}
