#pragma once

#include <PhxEngine/Renderer/DisplayService.h>
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

	class GltfDisplayService : public DisplayService
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
		GLFWwindow* m_glfwWindow;
		WindowData m_data;
	};
}

