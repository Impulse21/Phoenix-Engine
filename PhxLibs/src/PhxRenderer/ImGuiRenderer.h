#pragma once

#include "imgui.h"

#if false
#include <PhxRhi/RHICommon.h>
#include <PhxRenderer/RenderSubSystem.h>

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
	class ImGuiRenderSystem final : public IRenderSubSystem
	{
	public:
		void Initialize(IFileSystem* fs, void* windowHandle, bool enableDocking = false);
		void Finalize() override;

		void EnableDarkThemeColours();
		void BeginFrame();
		void EndFrame();
		void Render(RHI::CommandCtx* ctx);

		void* OnPreRender() override;
		void OnRender(RHI::CommandCtx* ctx, void* cachedData) override;

	private:
		// bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;
		RHI::Format m_indexFormat;

		RHI::DescriptorIndex m_fontTextureBindlessIndex = RHI::cInvalidDescriptorIndex;
		RHI::TextureHandle m_fontTexture;
		RHI::PipelineStateHandle m_pipeline;
	};
}
#endif
