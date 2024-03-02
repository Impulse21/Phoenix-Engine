#pragma once

#include <PhxEngine/Renderer/DisplayService.h>
#include <stdint.h>

struct GLFWwindow;

namespace PhxEngine
{
	struct WindowData
	{
		uint32_t Width;
		uint32_t Height;
	};

	class GltfDisplayService : public DisplayService
	{
	public:
		void Startup();
		void Shutdown();

		void Update();

		void Present();

		void* GetNativeWindowHandle() override;
		void* GetNativeWindow() override { return this->m_glfwWindow; }

	private:
		GLFWwindow* m_glfwWindow;
	};
}

