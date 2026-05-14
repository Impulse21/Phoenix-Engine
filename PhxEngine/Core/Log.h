#pragma once

#include <format>
#include <string_view>


namespace phx
{
    namespace Log
    {
        struct Channel { const char* name; };
        enum class Level { Trace, Info, Warning, Error };

        namespace Channels
        {
            inline constexpr Channel Engine { "Engine" };
            inline constexpr Channel App    { "App" };
            inline constexpr Channel RHI    { "RHI" };
            inline constexpr Channel Jobs   { "Jobs" };
        }

        void Initialize();
        void Shutdown();

        bool IsEnabled(Level /*level*/) { return true; } // TODO: Implement log levels

        void _WriteRaw(Level level, const Channel& channel, std::string_view message);

        template<typename... Args>
        void _Write(
            Level level,
            const Channel& channel,
            std::format_string<Args...> fmt,
            Args&&... args)
        {
            thread_local char t_buf[2048];

            auto result = std::format_to_n(
                t_buf,
                sizeof(t_buf) - 1,
                fmt,
                std::forward<Args>(args)...);

            *result.out = '\0';

            // TODO: Add assert
            PHX_ASSERT(result.size < sizeof(t_buf) - 1);
            _WriteRaw(level, channel, std::string_view(t_buf, result.out - t_buf));
        }
    }
}



#if defined(PHX_DEBUG)

    #define PHX_LOG_TRACE(ch, ...)  \
        do { if (phx::Log::IsEnabled(phx::Log::Level::Trace)) \
            phx::Log::_Write(phx::Log::Level::Trace, (ch), __VA_ARGS__); } while(0)

    #define PHX_LOG_INFO(ch, ...)   \
        do { if (phx::Log::IsEnabled(phx::Log::Level::Info))  \
            phx::Log::_Write(phx::Log::Level::Info,  (ch), __VA_ARGS__); } while(0)

    #define PHX_LOG_WARN(ch, ...)   \
        do { if (phx::Log::IsEnabled(phx::Log::Level::Warn))  \
            phx::Log::_Write(phx::Log::Level::Warn,  (ch), __VA_ARGS__); } while(0)

    #define PHX_LOG_ERROR(ch, ...)  \
        do { if (phx::Log::IsEnabled(phx::Log::Level::Error)) \
            phx::Log::_Write(phx::Log::Level::Error, (ch), __VA_ARGS__); } while(0)

#elif defined(PHX_DEBUG_INFO)

    #define PHX_LOG_TRACE(ch, ...)  do {} while(0)
    #define PHX_LOG_INFO(ch,  ...)  do {} while(0)

    #define PHX_LOG_WARN(ch, ...)   \
        do { phx::Log::_Write(phx::Log::Level::Warn,  (ch), __VA_ARGS__); } while(0)

    #define PHX_LOG_ERROR(ch, ...)  \
        do { phx::Log::_Write(phx::Log::Level::Error, (ch), __VA_ARGS__); } while(0)

#else

    #define PHX_LOG_TRACE(ch, ...)  do {} while(0)
    #define PHX_LOG_INFO(ch,  ...)  do {} while(0)
    #define PHX_LOG_WARN(ch,  ...)  do {} while(0)
    #define PHX_LOG_ERROR(ch, ...)  do {} while(0)

#endif