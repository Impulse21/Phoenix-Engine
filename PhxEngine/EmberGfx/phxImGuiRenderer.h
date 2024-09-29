#pragma once

#include "EmberGfx/phxGfxDevice.h"

struct ImGuiContext;
namespace phx::gfx
{
	class ImGuiRenderSystem
	{
	public:
	private:
		ImGuiContext* m_imguiContext;

		TextureHandle m_fontTexture;
		InputLayoutHandle m_inputLayout;
		GfxPipelineHandle m_pipeline;
	};
}