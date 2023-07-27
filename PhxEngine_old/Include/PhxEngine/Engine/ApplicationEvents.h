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

	class WindowKeyEvent : public Core::Event
	{
	public:
		WindowKeyEvent(int key, int scancode, int action, int mods)
			: m_key(key)
			, m_scancode(scancode)
			, m_action(action)
			, m_mods(mods)
		{}

		EVENT_CLASS_TYPE(WindowKeyEvent);


		int GetKey() const { this->m_key; }
		int GetScanCode() const { this->m_scancode; };
		int GetAction() const { this->m_action; };
		int GetMods() const { this->m_mods; }

	private:
		int m_key;
		int m_scancode;
		int m_action;
		int m_mods;
	};
}

