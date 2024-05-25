#pragma once

// This ignores all warnings raised inside External headers

#include <memory>
#include <format>
namespace phx::core
{
	namespace Log
	{
		void Initialize();

        enum class Level {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
            Critical
        };

        template<typename... Args>
        void log(Level level, const std::string& fmt, Args&&... args) 
        {
            auto& logger = getLogger();
            logger.log(convertLevel(level), std::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void trace(const std::string& fmt, Args&&... args) 
        {
            log(Level::Trace, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(const std::string& fmt, Args&&... args) 
        {
            log(Level::Debug, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void info(const std::string& fmt, Args&&... args) 
        {
            log(Level::Info, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warn(const std::string& fmt, Args&&... args) 
        {
            log(Level::Warn, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(const std::string& fmt, Args&&... args) 
        {
            log(Level::Error, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void critical(const std::string& fmt, Args&&... args) 
        {
            log(Level::Critical, fmt, std::forward<Args>(args)...);
        }
		
	}
}


#ifdef _DEBUG
// Core log macros
#define PHX_CORE_TRACE(...)    ::phx::core::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PHX_CORE_INFO(...)     ::phx::core::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PHX_CORE_WARN(...)     ::phx::core::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PHX_CORE_ERROR(...)    ::phx::core::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PHX_CORE_CRITICAL(...) ::phx::core::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros		   
#define PHX_TRACE(...)         ::phx::core::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PHX_INFO(...)          ::phx::core::Log::GetClientLogger()->info(__VA_ARGS__)
#define PHX_WARN(...)          ::phx::core::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PHX_ERROR(...)         ::phx::core::Log::GetClientLogger()->error(__VA_ARGS__)
#define PHX_CRITICAL(...)      ::phx::core::Log::GetClientLogger()->critical(__VA_ARGS__)
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