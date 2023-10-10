#pragma once

#include <PhxEngine/Core/Log.h>
#include <mutex>
#include <deque>

namespace PhxEngine::Editor
{
	class IWidgets
	{
	public:
		virtual ~IWidgets() = default;

		virtual void OnRender(bool displayWindow) = 0;
		virtual const std::string& GetName() = 0;
		virtual bool IsVisible() = 0;
	};

	class MenuBar : public IWidgets
	{
	public:
		MenuBar();
		void OnRender(bool displayWindow) override;

		const std::string& GetName() override { return "MenuBar"; }
		bool IsVisible() override { return true; }
	};

	class ProfilerWidget : public IWidgets
	{
	public:
		const std::string& GetName() override { return "Profiler"; }
		bool IsVisible() override { return true; }
	};

	class ConsoleLogWidget : public IWidgets
	{
	public:
		static constexpr std::string_view WidgetName = "Console";
		static constexpr size_t MaxLogMsgs = 1000;

	public:
		ConsoleLogWidget();
		void OnRender(bool displayWindow) override;
		const std::string& GetName() override { return "Console"; }
		bool IsVisible() override { return true; }

	private:
		void LogCallback(PhxEngine::Core::Log::LogEntry const& e);

	private:
		std::deque<PhxEngine::Core::Log::LogEntry> m_logEntries;
		std::mutex m_entriesLock;
	};
}

