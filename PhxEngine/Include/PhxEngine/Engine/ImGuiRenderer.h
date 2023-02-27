#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Engine/EngineRoot.h>

// Forward Declares
struct ImGuiContext;

namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
	}
	namespace Graphics
	{
		class ShaderFactory;
	}

	class ImGuiRenderer : public EngineRenderPass
	{
	public:
		ImGuiRenderer(IPhxEngineRoot* root);

		bool Initialize(Graphics::ShaderFactory& shaderFactory);
		void Update(Core::TimeStep const& timeStep) override;

		void Render() override;

		void OnWindowResize(WindowResizeEvent const& e) override;

	protected:
		void EnableDarkThemeColours();

		virtual void BuildUI() {}

		RHI::RenderPassHandle RenderPass;

	private:
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;
		RHI:: InputLayoutHandle m_inputLayout;
		RHI::GraphicsPipelineHandle m_pipeline;
	};
}

