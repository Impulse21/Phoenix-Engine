#pragma once

#include <PhxEngine/Core/Window.h>

struct GLFWwindow;

namespace PhxEngine::Core
{
	class WindowGlfw;
	struct WindowData
	{
		uint32_t Width;
		uint32_t Height;
		IWindow::EventCallBackFn EventCallback;
	};

	class WindowGlfw : public IWindow
	{
	public:
		WindowGlfw(PhxEngine::IPhxEngineRoot* engRoot, WindowSpecification const& spec);
		~WindowGlfw();

		void Initialize() override;

		void SetEventCallback(EventCallBackFn eventCallback) override { this->m_data.EventCallback = eventCallback; };

		void OnUpdate() override;

		uint32_t GetWidth() const override { return this->m_spec.Width; }
		uint32_t GetHeight() const override { return this->m_spec.Height; };

		void SetVSync(bool enableVysnc) override { this->m_vsyncEnabled = enableVysnc; }
		bool GetVSync() const override { return this->m_vsyncEnabled; }

		void SetResizeable(bool isResizeable) override;
		bool IsResizeable() const override;

		void Maximize() override;
		void CentreWindow() override;

		void* GetNativeWindowHandle() override;
		void* GetNativeWindow() override { return this->m_glfwWindow; }

		bool ShouldClose() override;

	private:
		PhxEngine::IPhxEngineRoot* m_root;

		const WindowSpecification m_spec;
		GLFWwindow* m_glfwWindow;

		WindowData m_data;

		bool m_vsyncEnabled;
		bool m_isResizeable;
		bool m_isVisible;
	
	};
}

