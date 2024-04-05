#pragma once

#include <PhxEngine/Core/Application.h>

// Forward Declares
struct ImGuiContext;

namespace PhxEngine
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer()
			: Layer("ImGuiLayer") {};
		~ImGuiLayer() = default;

		void OnAttach() override;
		void OnDetach() override;

		void Begin();
		void End();

		void SetDarkThemeColors();

		uint32_t GetActiveWidgetID() const;

	private:
		void CreatePipelineStateResources(RHI::GfxDevice& gfxDevice);

	private:
		EventHandler<WindowResizeEvent> m_resizeEventHandler;

		ImGuiContext* m_imguiContext;
		RHI::TextureHandle m_fontTexture;

		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;

		RHI::InputLayoutHandle m_inputLayout;
		RHI::GfxPipelineHandle m_pipeline;
	};
}

