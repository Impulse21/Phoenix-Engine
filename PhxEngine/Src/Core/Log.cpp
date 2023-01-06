#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <PhxEngine/Core/Log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace PhxEngine::Core;

std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Initialize()
{
	// Timestamp, name of logger.
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_CoreLogger = spdlog::stdout_color_mt("PHX_ENGINE");
	s_CoreLogger->set_level(spdlog::level::trace);

	s_ClientLogger = spdlog::stdout_color_mt("APPLICATION");
	s_ClientLogger->set_level(spdlog::level::trace);

}