#pragma once

#include <string>
#include <memory>
#include "spdlog/spdlog.h"

namespace PhxEngine::Core
{
	class Log
	{
	public:
		static void Initialize();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// -- Core log macros ----
#define LOG_CORE_TRACE(...) ::PhxEngine::Core::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_INFO(...)	::PhxEngine::Core::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)	::PhxEngine::Core::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...) ::PhxEngine::Core::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CORE_FATAL(...) ::PhxEngine::Core::Log::GetCoreLogger()->critical(__VA_ARGS__)


#define LOG_TRACE(...)	::PhxEngine::Core::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)	::PhxEngine::Core::Log::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)	::PhxEngine::Core::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	::PhxEngine::Core::Log::GetClientLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...)	::PhxEngine::Core::Log::GetClientLogger()->critical(__VA_ARGS__)