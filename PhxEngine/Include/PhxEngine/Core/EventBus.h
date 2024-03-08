#pragma once
#include <string>
#include <functional>
#include <memory>

#include <PhxEngine/Core/StringHash.h>

namespace PhxEngine
{
	struct Event
	{
	public:
		virtual ~Event() = default;
		virtual StringHash GetEventType() const = 0;

		bool Handled = false;
	};

#define EVENT_TYPE(eventType)																												\
	static PhxEngine::StringHash GetStaticEventType() { static PhxEngine::StringHash eventTypeStatic(#eventType); return eventTypeStatic; }	\
	PhxEngine::StringHash GetEventType() const override{ return GetStaticEventType(); };			 
	

	template<typename EventType>
	using EventHandler = std::function<void(EventType const& e)>;
	
	class IEventHandlerWrapper
	{
	public:
		virtual ~IEventHandlerWrapper() = default;
		void Invoke(Event const& e)
		{
			this->Call(e);
		}

		virtual const StringHash& GetType() const = 0;

	private:
		virtual void Call(Event const& e) = 0;
	};

	template<typename EventType>
	class EventHandlerWrapper : public IEventHandlerWrapper
	{
	public:
		explicit EventHandlerWrapper(EventHandler<EventType> const& handler)
			: m_handler(handler)
			, m_handlerType(m_handler.target_type().name()) {}

		const StringHash& GetType() const override { return this->m_handlerType; }

	private:
		void Call(Event const& e) override
		{
			if (e.GetEventType() == EventType::GetStaticEventType())
			{
				this->m_handler(static_cast<EventType const&>(e));
			}
		}

	private:
		EventHandler<EventType> m_handler;
		const StringHash m_handlerType;
	};


	/// Convenience macro to construct an EventHandler that points to a receiver object and its member function.
#define PHX_HANDLER(EventType, callback) std::make_unique< EventHandlerWrapper<EventType>>(callback);

	namespace EventBus
	{
		void Subscribe(StringHash eventId, std::unique_ptr<IEventHandlerWrapper>&& eventHandler);
		template<typename EventType>
		void Subscribe(EventHandler<EventType> const& callback)
		{
			auto handler = PHX_HANDLER(EventType, callback);
			Subscribe(EventType::GetStaticEventType(), std::move(handler));
		}

		void Unsubscribe(StringHash eventId, StringHash handlerId);
		template<typename EventType>
		void Unsubscribe(EventHandler<EventType> const& callback)
		{
			StringHash handlerName = StringHash(callback.target_type().name());
			Unsubscribe(EventType::GetStaticEventType(), handlerName);
		}
		void TriggerEvent( Event const& e);
		void QueueEvent(std::unique_ptr<Event>&& e);
		void DispatchEvents();
	}
}


