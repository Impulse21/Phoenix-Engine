#include "pch.h"
#include "phxImGuiRenderer.h"

#include "ImGui/imgui_impl_win32.h"
#include "phxGfxDevice.h"

using namespace phx::gfx;
namespace
{
	ImGuiContext* m_imguiContext;

	RHI::TextureHandle m_fontTexture;
	RHI::ShaderHandle m_vertexShader;
	RHI::ShaderHandle m_pixelShader;
	RHI::InputLayoutHandle m_inputLayout;
	RHI::GraphicsPipelineHandle m_pipeline;
}
