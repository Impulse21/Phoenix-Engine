#pragma once

#include "ImGui/imgui.h"
#include "phx/rhi/GfxDevice.h"

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
		void Initialize(rhi::GfxDevice* gfxDevice, IFileSystem* fs, bool enableDocking = false);
		void Finialize(rhi::GfxDevice* gfxDevice);

		void EnableDarkThemeColours();
		void BeginFrame();
		void Render(rhi::GfxCommandListRecorder& recorder);

	private:
		bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;

		rhi::DescriptorIndex m_fontTextureBindlessIndex = rhi::cInvalidDescriptorIndex;
		rhi::TextureHandle m_fontTexture;
		rhi::PipelineStateHandle m_pipeline;
	};
}