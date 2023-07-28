#pragma once

#include<PhxEngine/Core/Events.h>
#include <functional>
namespace PhxEngine::Core
{
	using EventListener = std::function<void(const Event&)>;
	namespace EventDispatcher
	{
		void AddEventListener(EventType type, EventListener listener);
		void RemoveEventListner(EventType type, EventListener listener);
		void DispatchEvent(Core::Event& event);
	}

};