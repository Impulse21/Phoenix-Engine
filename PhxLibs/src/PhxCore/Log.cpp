#include "PhxCore_pch.h"
#include "PhxCore/Log.h"

#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace phx;

#define FLUSH_ON_LOG true

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

    std::vector<spdlog::sink_ptr> logSinks;

    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks[0]->set_pattern("%^[%T] %n: %v%$");

    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("phx_engine.log", true));
    logSinks[1]->set_pattern("[%T] [%l] %n: %v");

    s_CoreLogger = std::make_shared<spdlog::logger>("Engine", begin(logSinks), end(logSinks));
    spdlog::register_logger(s_CoreLogger);
    s_CoreLogger->set_level(spdlog::level::trace);
#if FLUSH_ON_LOG
    s_CoreLogger->flush_on(spdlog::level::trace);
#endif

    // RHI Logger
    s_RhiLogger = std::make_shared<spdlog::logger>("RHI", begin(logSinks), end(logSinks));
    spdlog::register_logger(s_RhiLogger);
    s_RhiLogger->set_level(spdlog::level::trace);
#if FLUSH_ON_LOG
    s_RhiLogger->flush_on(spdlog::level::trace);
#endif

    // Client Logger
    s_ClientLogger = std::make_shared<spdlog::logger>("Application", begin(logSinks), end(logSinks));
    spdlog::register_logger(s_ClientLogger);
    s_ClientLogger->set_level(spdlog::level::trace);
#if FLUSH_ON_LOG
    s_ClientLogger->flush_on(spdlog::level::trace);
#endif
#endif

}