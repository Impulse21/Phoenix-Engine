#include <PhxEngine/Core/Log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace PhxEngine::Core;

namespace
{
	std::shared_ptr<spdlog::logger> m_CoreLogger;
	std::shared_ptr<spdlog::logger> m_ClientLogger;
}

void Log::Initialize()
{
	// Timestamp, name of logger.
	spdlog::set_pattern("%^[%T] %n: %v%$");

	m_CoreLogger = spdlog::stdout_color_mt("PHX_ENGINE");
	m_CoreLogger->set_level(spdlog::level::trace);

	m_ClientLogger = spdlog::stdout_color_mt("APPLICATION");
	m_ClientLogger->set_level(spdlog::level::trace);

}