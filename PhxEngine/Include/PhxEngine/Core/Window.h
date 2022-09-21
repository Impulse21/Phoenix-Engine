#pragma once

#include <memory>
#include <functional>
#include "PhxEngine/Core/Event.h"

namespace PhxEngine::Core
{

	class IWindow
	{
	public:
		using EventCallBackFn = std::function<void(Event&)>;
		virtual ~IWindow() = default;

		virtual void Initialize() = 0;
		virtual void OnUpdate() = 0;

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
		std::unique_ptr<IWindow> CreateGlfwWindow(WindowSpecification const& spec);
	}

}
