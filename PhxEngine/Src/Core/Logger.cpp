#include <PhxEngine/Core/Logger.h>
#include <mutex>
#include <iostream>

#ifdef WIN32
#include <Windows.h>
#endif
using namespace PhxEngine;

namespace
{
	std::vector<Logger::LogCallbackFn> m_logCallbaclks;

	std::mutex m_logLock;


	void PostMsgInternal(Logger::LogLevel logLevel, std::string_view category, std::string_view msg)
	{
		std::scoped_lock lock(m_logLock);

		Logger::LogEntry e = {
			.Level = logLevel,
		};

		std::string& msgStr = e.Msg;
		switch (logLevel)
		{
		default:
		case Logger::LogLevel::None:
			msgStr = "";
			break;
		case Logger::LogLevel::Info:
			msgStr = "[Info] ";
			break;
		case Logger::LogLevel::Warning:
			msgStr = "[Warning] ";
			break;
		case Logger::LogLevel::Error:
			msgStr = "[Error] ";
			break;
		}
		msgStr += "[" + std::string(category) + "] ";
		msgStr += msg;
		msgStr += '\n';


		for (auto& callback : m_logCallbaclks)
		{
			callback(e);
		}
	}
}

void Logger::Startup()
{
#ifdef _DEBUG

	m_logCallbaclks.emplace_back([](Logger::LogEntry const& entry) {

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
	m_logCallbaclks.emplace_back([](Logger::LogEntry const& entry) {
		OutputDebugStringA(entry.Msg.data());
		});

#endif
#endif

}

void PhxEngine::Logger::Shutdown()
{
	m_logCallbaclks.clear();
}

void PhxEngine::Logger::RegisterLogCallback(Logger::LogCallbackFn const& logTarget)
{
	std::scoped_lock _(m_logLock);
	m_logCallbaclks.push_back(logTarget);
}

void PhxEngine::Logger::PostMsg(std::string_view categroy, LogLevel logLEvel, std::string_view msg, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, msg);
	auto w = vsnprintf(buffer, sizeof(buffer), msg.data(), args);
	va_end(args);

	std::string_view v = buffer;
	PostMsgInternal(logLEvel, categroy, v);
}