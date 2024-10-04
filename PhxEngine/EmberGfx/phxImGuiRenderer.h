#pragma once

#include "EmberGfx/phxGfxDevice.h"
#include "ImGui/imgui.h"

struct ImGuiContext;
namespace phx::gfx
{
	class ImGuiRenderSystem
	{
	public:
		void Initialize(bool enableDocking = false);
		void EnableDarkThemeColours();
		void BeginFrame();
		void Render(CommandCtx& context);

	private:
		bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;

		DescriptorIndex m_fontTextureBindlessIndex = cInvalidDescriptorIndex;
		HandleOwner<Texture> m_fontTexture;
		HandleOwner<InputLayout> m_inputLayout;
		HandleOwner<GfxPipeline> m_pipeline;
	};
}