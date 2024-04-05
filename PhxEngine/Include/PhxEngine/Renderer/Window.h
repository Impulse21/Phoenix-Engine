#pragma once

#include <PhxEngine/Core/EventBus.h>

namespace PhxEngine
{
	class IWindow
	{
	public:
		virtual ~IWindow() = default;

	public:
		virtual void StartUp() = 0;
		virtual void OnUpdate() = 0;

		virtual void* GetNativeWindowHandle() = 0;
		virtual void* GetWindowImpl() = 0;
	};

	namespace WindowFactory
	{
		std::unique_ptr<IWindow> Create();
	}

	struct WindowResizeEvent : public Event
	{
		EVENT_TYPE(WindowResizeEvent);

		uint32_t Width;
		uint32_t Height;

		WindowResizeEvent(uint32_t width, uint32_t height)
			: Width(width)
			, Height(height)
		{
		}
	};

	struct WindowCloseEvent : public Event
	{
		EVENT_TYPE(WindowCloseEvent);
	};
}