#pragma once

#include <memory>
#include <functional>
#include <PhxEngine/Core/Events.h>
#include <string>

namespace PhxEngine::Core
{
	class IWindow
	{
	public:
		using EventCallBackFn = std::function<void(Event&)>;
		virtual ~IWindow() = default;

		virtual void Initialize() = 0;
		virtual void OnTick() = 0;

		virtual void SetEventCallback(EventCallBackFn eventCallback) = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetVSync(bool enableVysnc) = 0;
		virtual bool GetVSync() const = 0;

		virtual void SetResizeable(bool isResizeable) = 0;
		virtual bool IsResizeable() const = 0;

		virtual void Maximize() = 0;
		virtual void CentreWindow() = 0;

		virtual void* GetNativeWindowHandle() = 0;
		virtual void* GetNativeWindow() = 0;

		virtual void SetWindowTitle(std::string_view strView) = 0;
		virtual bool ShouldClose() = 0;
	};

	struct WindowSpecification
	{
		std::string Title;
		uint32_t Width = 0;
		uint32_t Height = 0;
		bool VSync = false;
		bool Fullscreen = false;
	};


	namespace WindowFactory
	{
		IWindow* CreateGlfwWindow(WindowSpecification const& spec);
	}

}
