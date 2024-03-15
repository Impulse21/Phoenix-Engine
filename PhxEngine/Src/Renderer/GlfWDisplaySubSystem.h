#pragma once

#include <PhxEngine/Renderer/DisplaySubSystem.h>
#include <stdint.h>

struct GLFWwindow;

namespace PhxEngine
{
	struct WindowData
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		std::string WindowTitle = "Phx Engine";

		bool VsyncEnabled = false;
		bool IsResizeable = false;
		bool IsVisible = false;
	};

	class GlfWDisplaySubSystem : public DisplaySubSystem
	{
	public:
		void Startup();
		void Shutdown();

		void Update();

		void Present();

		void* GetNativeWindowHandle() override;
		void* GetDisplayServiceImpl() override { return this; }

		GLFWwindow* GetGltfWindow() { return this->m_glfwWindow; }

	private:
		EventHandler<WindowResizeEvent> m_windowResizeHandler;
		GLFWwindow* m_glfwWindow;
		WindowData m_data;
	};
}

