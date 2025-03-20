#pragma once

#include "imgui.h"

#include <PhxRhi/RHITypes.h>
#include <PhxRenderer/RenderSystem.h>

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
	class ImGuiRenderSystem final : public IRenderSystem
	{
	public:
		void Initialize(IFileSystem* fs, void* windowHandle, bool enableDocking = false);
		void Finialize();

		void EnableDarkThemeColours();
		void BeginFrame();
		void Render(rhi::CommandCtx* ctx);

		void* OnPreRender() override;
		void OnRender(rhi::CommandCtx* ctx, void* cachedData) override;

	private:
		// bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;
		rhi::Format m_indexFormat;
		std::array<ImGuiContext*, 2> m_imguiContexts;
		rhi::DescriptorIndex m_fontTextureBindlessIndex = rhi::cInvalidDescriptorIndex;
		rhi::TextureHandle m_fontTexture;
		rhi::PipelineStateHandle m_pipeline;
	};
}
