#include "pch.h"
#include "phxLog.h"

#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace phx;


namespace
{
	std::shared_ptr<spdlog::logger> g_coreLogger;
	std::shared_ptr<spdlog::logger> g_clientLogger;
	void LogInternal(spdlog::logger& logger, Log::Level level, std::string_view msg)
	{
		switch (level)
		{
		case Log::Level::Trace:
			logger.trace(msg);
			break;
		case Log::Level::Info:
			logger.info(msg);
			break;
		case Log::Level::Debug:
			logger.debug(msg);
			break;
		case Log::Level::Warn:
			logger.warn(msg);
			break;
		case Log::Level::Error:
			logger.error(msg);
			break;
		case Log::Level::Critical:
		default:
			logger.critical(msg);
			break;
		}
	}
}

void phx::Log::Initialize()
{
#if false
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("phxLog.log", true));

	logSinks[0]->set_pattern("%^[%T] %n: %v%$");
	logSinks[1]->set_pattern("[%T] [%l] %n: %v");

	g_coreLogger = std::make_shared<spdlog::logger>("Phx Engine", begin(logSinks), end(logSinks));
	spdlog::register_logger(g_coreLogger);
	g_coreLogger->set_level(spdlog::level::trace);
	g_coreLogger->flush_on(spdlog::level::trace);

	g_clientLogger = std::make_shared<spdlog::logger>("Application", begin(logSinks), end(logSinks));
	spdlog::register_logger(g_clientLogger);
	g_clientLogger->set_level(spdlog::level::trace);
	g_clientLogger->flush_on(spdlog::level::trace);
#else

	// Timestamp, name of logger.
	spdlog::set_pattern("%^[%T] %n: %v%$");

	g_coreLogger = spdlog::stdout_color_mt("Engine");
	g_coreLogger->set_level(spdlog::level::trace);

	g_clientLogger = spdlog::stdout_color_mt("Application");
	g_clientLogger->set_level(spdlog::level::trace);
#endif

}

void phx::Log::Log(LogType type, Level level, std::string_view msg)
{
	switch (type)
	{
	case LogType::App:
		LogInternal(*g_clientLogger, level, msg);
		break;
	case LogType::Engine:
	default:
		LogInternal(*g_coreLogger, level, msg);
		break;
	};
}