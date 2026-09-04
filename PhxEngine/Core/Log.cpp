#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace phx;

namespace
{
    std::shared_ptr<spdlog::logger> s_logger;

    spdlog::level::level_enum ToSpdlog(Log::Level level)
    {
        switch (level)
        {
            case Log::Level::Trace:     return spdlog::level::trace;
            case Log::Level::Info:      return spdlog::level::info;
            case Log::Level::Warning:   return spdlog::level::warn;
            case Log::Level::Error:     return spdlog::level::err;
        }

        return spdlog::level::info;
    }
}

void phx::Log::Initialize() 
{

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>("PhxEngine.log", true);

    s_logger = std::make_shared<spdlog::logger>("PhxEngine", spdlog::sinks_init_list{ consoleSink, fileSink });

    s_logger->set_pattern("[%T] [%^%l%$] %v");
    s_logger->set_level(spdlog::level::trace);
}

void phx::Log::Shutdown() 
{
    s_logger.reset();
    spdlog::shutdown();
}

void phx::Log::_WriteRaw(Log::Level level, const Log::Channel& channel,
                         std::string_view message)
{
    s_logger->log(ToSpdlog(level), "[{}] {}", channel.name, message);
}
