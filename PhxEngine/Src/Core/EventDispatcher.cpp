#include <PhxEngine/Core/EventDispatcher.h>

#include <unordered_map>
#include <PhxEngine/Core/Vector.h>

using namespace PhxEngine::Core;

namespace
{
	std::unordered_map<EventType, Phx::FlexArray<EventListener>> m_events;
}

void EventDispatcher::AddEventListener(EventType type, EventListener listener)
{
	m_events[type].push_back(listener);
}

void PhxEngine::Core::EventDispatcher::RemoveEventListner(EventType type, EventListener listener)
{
}

void PhxEngine::Core::EventDispatcher::DispatchEvent(Core::Event& event)
{
	auto& listeners = m_events[event.GetEventType()];
	for (const auto& listener : listeners) 
	{
		listener(event);
	}
}

