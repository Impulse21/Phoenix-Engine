#pragma once

// This ignores all warnings raised inside External headers

#include <memory>
#include <format>
#include <spdlog/logger.h>

namespace phx
{
	class Log
	{
	public:
		static void Initialize();
		inline static spdlog::logger* GetCoreLogger() { return s_CoreLogger.get(); }
		inline static spdlog::logger* GetClientLogger() { return s_ClientLogger.get(); }
	private:
		inline static std::shared_ptr<spdlog::logger> s_CoreLogger;
		inline static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#ifdef _DEBUG

// Core log macros
#define PHX_CORE_TRACE(...)    ::phx::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PHX_CORE_INFO(...)     ::phx::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PHX_CORE_WARN(...)     ::phx::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PHX_CORE_ERROR(...)    ::phx::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PHX_CORE_CRITICAL(...) ::phx::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros		   
#define PHX_TRACE(...)			::phx::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PHX_INFO(...)			::phx::Log::GetClientLogger()->info(__VA_ARGS__)
#define PHX_WARN(...)			::phx::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PHX_ERROR(...)			::phx::Log::GetClientLogger()->error(__VA_ARGS__)
#define PHX_CRITICAL(...)		::phx::Log::GetClientLogger()->critical(__VA_ARGS__)

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