#pragma once

#include "imgui.h"
#include "phx/rhi/RHITypes.h"

namespace phx
{
	class IFileSystem;
}
struct ImGuiContext;

namespace phx::rhi
{
	class CommandCtx;
}
namespace phx::gfx
{
	class ImGuiRenderSystem
	{
	public:
		void Initialize(IFileSystem* fs, void* windowHandle, bool enableDocking = false);
		void Finialize();

		void EnableDarkThemeColours();
		void BeginFrame();
		void Render(CommandCtx* ctx);

	private:
		// bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;

		rhi::DescriptorIndex m_fontTextureBindlessIndex = rhi::cInvalidDescriptorIndex;
		rhi::TextureHandle m_fontTexture;
		rhi::PipelineStateHandle m_pipeline;
	};
}
