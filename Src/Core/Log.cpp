#include <PhxEngine\Core\Log.h>

#include <spdlog/spdlog.h>


#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace spdlog;
using namespace PhxEngine;


std::unique_ptr<SpdLogger> LogSystem::s_CoreLogger;
std::unique_ptr<SpdLogger> LogSystem::s_ClientLogger;

void LogSystem::Initialize()
{	
	// Timestamp, name of logger.
	spdlog::set_pattern("%^[%T] %n: %v%$");

	{
		auto spdLog = spdlog::stdout_color_mt("PhxEngine");
		spdLog->set_level(spdlog::level::trace);

		s_CoreLogger = std::make_unique<SpdLogger>(spdLog);
	}

	{
		auto spdLog = spdlog::stdout_color_mt("Application");
		spdLog->set_level(spdlog::level::trace);

		s_ClientLogger = std::make_unique<SpdLogger>(spdLog);
	}
}

void PhxEngine::SpdLogger::PrintMsg(LogLevel level, const char* msg)
{
	switch (level)
	{
	case LogLevel::Fatal:
		this->m_spdLogger->critical(msg);
		break;

	case LogLevel::Error:
		this->m_spdLogger->error(msg);
		break;

	case LogLevel::Warn:
		this->m_spdLogger->warn(msg);
		break;

	case LogLevel::Info:
		this->m_spdLogger->info(msg);
		break;

	case LogLevel::Trace:
		this->m_spdLogger->trace(msg);
		break;
	}
}
