#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

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

		bool Initialize();
		void Tick(Core::TimeStep const& timeStep);

		void Render();

	protected:
		void EnableDarkThemeColours();

		virtual void BuildUI() {}

		RHI::RenderPassHandle RenderPass;

	private:
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;
		RHI::InputLayoutHandle m_inputLayout;
		RHI::GfxPipelineHandle m_pipeline;
	};
}

