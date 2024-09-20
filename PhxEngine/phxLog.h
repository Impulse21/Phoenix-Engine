#pragma once

// This ignores all warnings raised inside External headers

#include <memory>
#include <format>
namespace phx
{
    namespace Log
    {
        void Initialize();

        enum class Level
        {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
            Critical
        };
        enum class LogType
        {
            Engine = 0,
            App,
        };

        void Log(LogType type, Level level, std::string_view msg);

        template<LogType _Type, typename... Args>
        void Log(Level level, const std::format_string<Args...> fmt, Args&&... args)
        {
            Log(_Type, level, std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template<LogType _Type, typename... Args>
        void Trace(const std::format_string<Args...> fmt, Args&&... args)
        {
            Log<_Type>(Level::Trace, fmt, std::forward<Args>(args)...);
        }

        template<LogType _Type, typename... Args>
        void Debug(const std::format_string<Args...> fmt, Args&&... args)
        {
            Log<_Type>(Level::Debug, fmt, std::forward<Args>(args)...);
        }

        template<LogType _Type, typename... Args>
        void Info(const std::format_string<Args...> fmt, Args&&... args)
        {
            Log<_Type>(Level::Info, fmt, std::forward<Args>(args)...);
        }

        template<LogType _Type, typename... Args>
        void Warn(const std::format_string<Args...> fmt, Args&&... args)
        {
            Log<_Type>(Level::Warn, fmt, std::forward<Args>(args)...);
        }

        template<LogType _Type, typename... Args>
        void Error(const std::format_string<Args...> fmt, Args&&... args)
        {
            Log<_Type>(Level::Error, fmt, std::forward<Args>(args)...);
        }

        template<LogType _Type, typename... Args>
        void Critical(const std::format_string<Args...> fmt, Args&&... args)
        {
            Log<_Type>(Level::Critical, fmt, std::forward<Args>(args)...);
        }
    }
}


#ifdef _DEBUG
// Core log macros
#define PHX_CORE_TRACE(...)    ::phx::Log::Trace<phx::Log::LogType::Engine>(__VA_ARGS__)
#define PHX_CORE_INFO(...)     ::phx::Log::Info<phx::Log::LogType::Engine>(__VA_ARGS__)
#define PHX_CORE_WARN(...)     ::phx::Log::Warn<phx::Log::LogType::Engine>(__VA_ARGS__)
#define PHX_CORE_ERROR(...)    ::phx::Log::Error<phx::Log::LogType::Engine>(__VA_ARGS__)
#define PHX_CORE_CRITICAL(...) ::phx::Log::Critical<phx::Log::LogType::Engine>(__VA_ARGS__)

// Client log macros		   
#define PHX_TRACE(...)         ::phx::Log::Trace<phx::Log::LogType::App>(__VA_ARGS__)
#define PHX_INFO(...)          ::phx::Log::Info<phx::Log::LogType::App>(__VA_ARGS__)
#define PHX_WARN(...)          ::phx::Log::Warn<phx::Log::LogType::App>(__VA_ARGS__)
#define PHX_ERROR(...)         ::phx::Log::Error<phx::Log::LogType::App>(__VA_ARGS__)
#define PHX_CRITICAL(...)      ::phx::Log::Critical<phx::Log::LogType::App>(__VA_ARGS__)
#else
// Core log macros
#define PHX_CORE_TRACE(...)    
#define PHX_CORE_INFO(...)     
#define PHX_CORE_WARN(...)     
#define PHX_CORE_ERROR(...)    
#define PHX_CORE_CRITICAL(...) 

// Client log macros
#define PHX_TRACE(...)         
#define PHX_INFO(...)          
#define PHX_WARN(...)          
#define PHX_ERROR(...)         
#define PHX_CRITICAL(...)      
#endif