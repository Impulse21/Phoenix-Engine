#pragma once

#include "imgui.h"
#include "phx/rhi/RHICore.h"

namespace phx
{
	class IFileSystem;
}
struct ImGuiContext;

namespace phx::gfx
{
	class ImGuiRenderSystem
	{
	public:
		void Initialize(IFileSystem* fs, bool enableDocking = false);
		void Finialize();

		void EnableDarkThemeColours();
		void BeginFrame();
		void Render();

	private:
		// bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;

		// rhi::DescriptorIndex m_fontTextureBindlessIndex = rhi::cInvalidDescriptorIndex;
		rhi::TextureHandle m_fontTexture;
		rhi::PipelineStateHandle m_pipeline;
	};
}
