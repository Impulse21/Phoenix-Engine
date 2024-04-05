#pragma once

#include <PhxEngine/Core/Logger.h>
#include <string>
#include <mutex>
#include <deque>

namespace PhxEditor
{
	class IWidget
	{
	public:
		virtual ~IWidget() = default;

		virtual void OnRender(bool displayWindow) = 0;
		virtual bool IsVisible() = 0;
	};

	template <typename T>
	struct WidgetTitle {
		static_assert(sizeof(T) == 0, "Unknown Type");
	};

	class MenuBar : public IWidget
	{
	public:
		MenuBar();
		void OnRender(bool displayWindow) override;

		bool IsVisible() override { return true; }
	};

	template <>
	struct WidgetTitle<MenuBar> {
		static constexpr const char* name()
		{
			return "MenuBar";
		}
	};

	class ProfilerWidget : public IWidget
	{
	public:
		void OnRender(bool displayWindow) override {};

		bool IsVisible() override { return true; }
	};

	template <>
	struct WidgetTitle<ProfilerWidget> {
		static constexpr const char* name()
		{
			return "Profiler";
		}
	};

	class ConsoleLogWidget : public IWidget
	{
	public:
		static constexpr std::string_view WidgetName = "Console";
		static constexpr size_t MaxLogMsgs = 1000;

	public:
		ConsoleLogWidget();
		void OnRender(bool displayWindow) override;
		bool IsVisible() override { return true; }

	private:
		void LogCallback(PhxEngine::Logger::LogEntry const& e);

	private:
		std::deque<PhxEngine::Logger::LogEntry> m_logEntries;
		std::mutex m_entriesLock;
	};

	template <>
	struct WidgetTitle<ConsoleLogWidget> {
		static constexpr const char* name()
		{
			return "Console";
		}
	};

}

