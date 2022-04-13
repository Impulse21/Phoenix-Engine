#pragma once

#include <memory>
#include <PhxEngine/ImGui/ImGuiLayer.h>

namespace PhxEngine::Editor
{
	class EditorLayer;
	class GuiLayer : public Debug::ImGuiLayer
	{
	public:
		GuiLayer(
			RHI::IGraphicsDevice* graphicsDevice,
			GLFWwindow* gltfWindow,
			std::shared_ptr<EditorLayer> editorLayer)
			: Debug::ImGuiLayer(graphicsDevice, gltfWindow)
			, m_editorLayer(editorLayer)
		{

		}

		~GuiLayer() = default;

	protected:
		void BuildUI();

	private:
		std::shared_ptr<EditorLayer> m_editorLayer;
	};
}
