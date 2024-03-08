#include <PhxEngine/Core/EventBus.h>

#include <unordered_map>
#include <vector>
#include <deque>

using namespace PhxEngine;

namespace
{
	std::unordered_map<StringHash, std::vector<std::unique_ptr<IEventHandlerWrapper>>> m_subscribers;
	std::deque<std::unique_ptr<Event>> m_eventQueue;
}

void PhxEngine::EventBus::Subscribe(
	StringHash eventId,
	std::unique_ptr<IEventHandlerWrapper>&& eventHandler)
{
	auto subscribersItr = m_subscribers.find(eventId);
	if (subscribersItr == m_subscribers.end())
	{
		m_subscribers[eventId].emplace_back(std::move(eventHandler));
		return;
	}

	auto& handlers = subscribersItr->second;
	for (auto& itr : handlers)
	{
		if (itr->GetType() == eventHandler->GetType())
		{
			return;
		}
	}

	handlers.emplace_back(std::move(eventHandler));
}

void EventBus::Unsubscribe(StringHash eventId, StringHash handlerId)
{
	auto& handlers = m_subscribers[eventId];
	for (auto itr = handlers.begin(); itr != handlers.end(); itr++)
	{
		if (itr->get()->GetType() == handlerId)
		{
			itr = handlers.erase(itr);
			return;
		}
	}

}

void EventBus::TriggerEvent(Event const& e)
{
	for (auto& handler : m_subscribers[e.GetEventType()])
	{
		handler->Invoke(e);
	}
}

void EventBus::QueueEvent(std::unique_ptr<Event>&& e)
{
	m_eventQueue.emplace_back(std::move(e));
}

void EventBus::DispatchEvents()
{
	while(!m_eventQueue.empty())
	{
		auto& e = m_eventQueue.front();
		if (!e->Handled)
		{
			TriggerEvent(*e);
		}

		m_eventQueue.pop_front();
	}
}
