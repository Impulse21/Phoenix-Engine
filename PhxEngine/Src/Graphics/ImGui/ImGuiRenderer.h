#pragma once

#include <memory>

#include "PhxEngine/Engine/Layer.h"
#include "PhxEngine/Core/Platform.h"
#include "PhxEngine/RHI/PhxRHI.h"

// Forward Declares
struct ImGuiContext;

namespace PhxEngine::Graphics
{
	class ShaderStore;

	class ImGuiRenderer : public AppLayer
	{
	public:
		ImGuiRenderer() = default;
		virtual ~ImGuiRenderer() = default;

		void OnAttach() override;
		void OnDetach() override;

		void BeginFrame();
		void OnCompose(RHI::CommandListHandle cmd) override;

	private:
		void CreatePipelineStateObject(
			RHI::IGraphicsDevice* graphicsDevice);

		void SetDarkThemeColors();

	private:
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::GraphicsPSOHandle m_pso;
	};
}