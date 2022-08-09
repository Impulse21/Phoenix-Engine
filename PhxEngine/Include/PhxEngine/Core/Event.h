#pragma once

#include <string>


#define EVENT_CLASS_TYPE(type) static Core::EventType GetStaticType() { return Core::EventType::type; }\
								virtual Core::EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

namespace PhxEngine::Core
{
	enum class EventType
	{
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		bool IsHandled = false;

		virtual const char* GetName() const = 0;
		virtual EventType GetEventType() const = 0;
	};
}

