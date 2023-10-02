#include <PhxEngine/Core/Log.h>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <iostream>

#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/Containers.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/Object.h>

using namespace PhxEngine::Core;

namespace
{
	constexpr std::unordered_map<Log::LogEntry, const char*> LogLevelTag =
	{
		{ Log::LogLevel::None, ""},
		{ Log::LogLevel::Info, "[Info]"},
		{ Log::LogLevel::Error, "[Error]"},
	};

	std::vector<std::unique_ptr<Log::ILogTarget>> m_logTargets;
	
	// An array of data or an allocator
	FlexArray<Log::LogEntry> m_logEntires;
	std::mutex m_logLock;

	class CoutLogTarget : public Log::ILogTarget, public Object
	{
	public:
		CoutLogTarget() = default;

		void Flush(Span<Log::LogEntry> logEntries) override
		{
			for (auto& entry : logEntries)
			{
				std::count << LogLevelTag[entry.Level] << " - " << entry.Msg.data() << std::endl;
			}
		}
	};
}

void Log::Initialize()
{
#ifdef _DEBUG
	m_logTargets.push_back(std::make_unique<CoutLogTarget>());
#endif

}

void PhxEngine::Core::Log::Finialize()
{
	Log::Flush();
	m_logTargets.clear();
}

void PhxEngine::Core::Log::RegisterTarget(std::unique_ptr<ILogTarget>&& logTarget)
{
	std::scoped_lock _(m_logLock);
	m_logTargets.push_back(std::move(logTarget));
}

void PhxEngine::Core::Log::Log(LogLevel logLEvel, std::string_view msg)
{
	LogEntry e = {
		.Level = logLEvel,
		.Msg = msg
	};

	m_logEntires.push_back(e);
}

void PhxEngine::Core::Log::Flush()
{
	std::scoped_lock _(m_logLock);
	Core::Span<LogEntry> entires(m_logEntires.data(), m_logEntires.size());
	for (auto& target : m_logTargets)
	{
		target->Flush(entires);
	}

	m_logEntires.clear();
}
