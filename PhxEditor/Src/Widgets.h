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

		virtual void OnImGuiRender(bool displayWindow) = 0;
		virtual bool IsVisible() = 0;
	};

	template <typename T>
	struct WidgetTitle {
		static_assert(sizeof(T) == 0, "Unknown Type");
	};


	class ConsoleLogWidget : public IWidget
	{
	public:
		static constexpr size_t MaxLogMsgs = 1000;

	public:
		ConsoleLogWidget();
		void OnImGuiRender(bool displayWindow) override;
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

