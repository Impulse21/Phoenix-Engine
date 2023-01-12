#pragma once

#include "PhxEngine/Core/Event.h"

namespace PhxEngine
{
	class WindowCloseEvent : public Core::Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose);
		

	};

	class WindowResizeEvent : public Core::Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_width(width)
			, m_height(height)
		{}

		EVENT_CLASS_TYPE(WindowResize);

		uint32_t GetWidth() const { return this->m_width; }
		uint32_t GetHeight() const { return this->m_height; }

	private:
		uint32_t m_width;
		uint32_t m_height;
	};
}

