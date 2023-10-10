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
	};

	class ConsoleLogWidget : public IWidgets
	{
	public:
		static constexpr std::string_view WidgetName = "Console";
		static constexpr size_t MaxLogMsgs = 1000;

	public:
		ConsoleLogWidget();
		void OnRender(bool displayWindow) override;

	private:
		void LogCallback(PhxEngine::Core::Log::LogEntry const& e);

	private:
		std::deque<PhxEngine::Core::Log::LogEntry> m_logEntries;
		std::mutex m_entriesLock;
	};
}

