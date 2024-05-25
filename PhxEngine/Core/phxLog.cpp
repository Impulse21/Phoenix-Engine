#include "pch.h"

#include "phxLog.h"

#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace
{
	std::shared_ptr<spdlog::logger> g_coreLogger;
	std::shared_ptr<spdlog::logger> g_clientLogger;
}

void phx::core::Log::Initialize()
{
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Hazel.log", true));

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
}
