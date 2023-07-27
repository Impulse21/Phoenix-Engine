#pragma once

#include <memory>
#include <functional>

#define EVENT_CLASS_TYPE(type) static Core::EventType GetStaticType() { return Core::EventType::type; }\
								virtual Core::EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

namespace PhxEngine::Core
{
	enum class EventType
	{
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowKeyEvent
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		bool IsHandled = false;

		virtual const char* GetName() const = 0;
		virtual EventType GetEventType() const = 0;
	};


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

		virtual void SetWindowTitle(std::string_view strView) = 0;
		virtual bool ShouldClose() = 0;
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
