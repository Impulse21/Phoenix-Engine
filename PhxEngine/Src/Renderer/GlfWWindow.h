#pragma once

#include <PhxEngine/Renderer/Window.h>
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

	class GlfWWindow final : public IWindow
	{
	public:
		GlfWWindow();
		~GlfWWindow() override;

		void StartUp() override;
		void OnUpdate() override;

		void* GetNativeWindowHandle() override;
		void* GetNativeWindow() override { return this->GetGltfWindow(); }
		void* GetImpl() override { return this; }

		GLFWwindow* GetGltfWindow() { return this->m_glfwWindow; }

	private:
		void Initialize();
		void Finalize();

	private:
		GLFWwindow* m_glfwWindow;
		WindowData m_data;
	};
}

