#include <PhxEngine/Core/Log.h>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <iostream>

#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/Containers.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/Object.h>

#ifdef WIN32
#include <Windows.h>
#endif
using namespace PhxEngine::Core;

namespace
{
	std::vector<Log::LogCallbackFn> m_logCallbaclks;

	std::mutex m_logLock;
}

void Log::Initialize()
{
#ifdef _DEBUG

	m_logCallbaclks.emplace_back([](Log::LogEntry const& entry) {

		switch (entry.Level)
		{
		default:
		case LogLevel::None:
		case LogLevel::Info:
			std::cout << entry.Msg;
			break;
		case LogLevel::Warning:
			std::clog << entry.Msg;
			break;
		case LogLevel::Error:
			std::cerr << entry.Msg;
			break;
		}
	});

#ifdef WIN32
	m_logCallbaclks.emplace_back([](Log::LogEntry const& entry) {
		OutputDebugStringA(entry.Msg.data());
	});

#endif
#endif

}

void PhxEngine::Core::Log::Finialize()
{
	m_logCallbaclks.clear();
}

void PhxEngine::Core::Log::RegisterLogCallback(Log::LogCallbackFn const& logTarget)
{
	std::scoped_lock _(m_logLock);
	m_logCallbaclks.push_back(logTarget);
}

void PhxEngine::Core::Log::PostMsg(LogLevel logLevel, std::string_view msg)
{
	std::scoped_lock lock(m_logLock);

	LogEntry e = {
		.Level = logLevel,
	};

	std::string& msgStr = e.Msg;
	switch (logLevel)
	{
	default:
	case LogLevel::None:
		msgStr = "";
		break;
	case LogLevel::Info:
		msgStr = "[Info] ";
		break;
	case LogLevel::Warning:
		msgStr = "[Warning] ";
		break;
	case LogLevel::Error:
		msgStr = "[Error] ";
		break;
	}
	msgStr += msg;
	msgStr += '\n';


	for (auto& callback : m_logCallbaclks)
	{
		callback(e);
	}
}