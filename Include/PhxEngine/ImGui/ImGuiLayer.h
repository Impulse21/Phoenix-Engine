#pragma once

#include <memory>

#include <PhxEngine/Core/Layer.h>
#include <PhxEngine/RHI/PhxRHI.h>

// Forward Declares
struct ImGuiContext;
struct GLFWwindow;


namespace PhxEngine::Debug
{
	class ImGuiLayer : public Core::Layer
	{
	public:
		ImGuiLayer(
			RHI::IGraphicsDevice* graphicsDevice,
			GLFWwindow* gltfWindow);
		virtual ~ImGuiLayer() = default;

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Core::TimeStep const& ts) override final;
		void OnRender(RHI::TextureHandle& currentBuffer) override final;

	protected:
		virtual void BuildUI() = 0;

	private:
		void CreatePipelineStateObject();

	private:
		GLFWwindow* m_gltfWindow;
		RHI::IGraphicsDevice* m_graphicsDevice;
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::GraphicsPSOHandle m_pso;
		RHI::CommandListHandle m_commandList;
	};
}