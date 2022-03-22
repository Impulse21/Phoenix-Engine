#pragma once


#include <PhxEngine/ImGui/ImGuiLayer.h>

namespace PhxEngine::Editor
{
	class GuiLayer : public Debug::ImGuiLayer
	{
	public:
		GuiLayer(
			RHI::IGraphicsDevice* graphicsDevice,
			GLFWwindow* gltfWindow)
			: Debug::ImGuiLayer(graphicsDevice, gltfWindow)
		{

		}

		~GuiLayer() = default;

	protected:
		void BuildUI();

	};
}
