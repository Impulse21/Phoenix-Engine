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

	class ImGuiRenderer : public EngineRenderPass
	{
	public:
		ImGuiRenderer(RHI::IGraphicsDevice* gfxDevice, Core::IWindow* window);

		void Update(Core::TimeStep const& timeStep) override;

		void Render() override;

	protected:
		void EnableDarkThemeColours();

		virtual void BuildUI() {}

	private:
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;
		RHI::GraphicsPipelineHandle m_pipeline;
	};
}

