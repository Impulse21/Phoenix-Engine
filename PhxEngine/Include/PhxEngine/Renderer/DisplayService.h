#pragma once

#include <PhxEngine/Core/EventBus.h>

namespace PhxEngine
{
	class DisplayService
	{
	public:
		inline static DisplayService* Ptr;
		virtual ~DisplayService() = default;

	public:
		virtual void Startup() = 0;
		virtual void Shutdown() = 0;

		virtual void Update() = 0;

		virtual void Present() = 0;

		virtual void* GetNativeWindowHandle() = 0;
		virtual void* GetDisplayServiceImpl() = 0;
	};

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