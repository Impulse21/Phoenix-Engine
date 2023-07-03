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

	class ImGuiRenderer
	{
	public:
		ImGuiRenderer();

		bool Initialize(Graphics::ShaderFactory& shaderFactory);
		void Update(Core::TimeStep const& timeStep);

		void Render();

		void OnWindowResize(WindowResizeEvent const& e);

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

