#pragma once

#include "ImGui/imgui.h"
#include "EmberGfx/phxGfxDeviceResources.h"
#include "EmberGfx/phxEmber.h"

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
		void Initialize(GpuDevice* gfxDevice, IFileSystem* fs, bool enableDocking = false);
		void EnableDarkThemeColours();
		void BeginFrame();
		void Render(ICommandCtx* context);

	private:
		bool m_isFontTextureUploaded = false;
		ImGuiContext* m_imguiContext;

		DescriptorIndex m_fontTextureBindlessIndex = cInvalidDescriptorIndex;
		TextureHandle m_fontTexture;
		InputLayoutHandle m_inputLayout;
		PipelineStateHandle m_pipeline;
	};
}