#pragma once

#include "EmberGfx/phxGfxDevice.h"

struct ImGuiContext;
namespace phx::gfx
{
	class ImGuiRenderSystem
	{
	public:
		void Initialize(bool enableDocking = false);
		void EnableDarkThemeColours();
		void BeginFrame();
		void Render();

	private:
		ImGuiContext* m_imguiContext;

		TextureHandle m_fontTexture;
		InputLayoutHandle m_inputLayout;
		GfxPipelineHandle m_pipeline;
	};
}