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
	namespace Renderer
	{
		class RgBuilder;
	}
}

namespace PhxEngine::Renderer
{
	namespace ImGuiRenderer
	{
		void Initialize(Core::IWindow* window, RHI::Format swapChainFormat, bool enableDocking = false);
		void Finalize();
		void EnableDarkThemeColours();
		
		void BeginFrame();
		void Render(RgBuilder& builder);
	}
#if 0
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

	private:
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;
		RHI::InputLayoutHandle m_inputLayout;
		RHI::GfxPipelineHandle m_pipeline;
};
#endif
}

