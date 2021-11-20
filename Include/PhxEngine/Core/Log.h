#pragma once

#include <memory>

namespace spdlog
{
	class logger;
}

namespace PhxEngine
{
	enum class LogLevel
	{
		Fatal = 0,
		Error,
		Warn,
		Info,
		Trace
	};

	class SpdLogger
	{
	public:
		SpdLogger(std::shared_ptr<spdlog::logger> logger)
			: m_spdLogger(std::move(logger))
		{}

		void PrintMsg(LogLevel level, const char* msg, ...);

	private:
		std::shared_ptr<spdlog::logger> m_spdLogger;
	};

	class LogSystem
	{
	public:
		static void Initialize();

		inline static SpdLogger* GetCoreLogger() { return s_CoreLogger.get(); }
		inline static SpdLogger* GetClientLogger() { return s_ClientLogger.get(); }

	private:
		static std::unique_ptr<SpdLogger> s_CoreLogger;
		static std::unique_ptr<SpdLogger> s_ClientLogger;
	};
}

// -- Core log macros ----
#define LOG_CORE_TRACE(msg, ...) ::PhxEngine::LogSystem::GetCoreLogger()->PrintMsg(PhxEngine::LogLevel::Trace, msg, __VA_ARGS__)
#define LOG_CORE_INFO(msg, ...)	::PhxEngine::LogSystem::GetCoreLogger()->PrintMsg(PhxEngine::LogLevel::Info, msg, __VA_ARGS__)
#define LOG_CORE_WARN(msg, ...)	::PhxEngine::LogSystem::GetCoreLogger()->PrintMsg(PhxEngine::LogLevel::Warn, msg, __VA_ARGS__)
#define LOG_CORE_ERROR(msg, ...) ::PhxEngine::LogSystem::GetCoreLogger()->PrintMsg(PhxEngine::LogLevel::Error, msg, __VA_ARGS__)
#define LOG_CORE_FATAL(msg, ...) ::PhxEngine::LogSystem::GetCoreLogger()->PrintMsg(PhxEngine::LogLevel::Fatal, msg, __VA_ARGS__)


#define LOG_TRACE(msg, ...)	::PhxEngine::LogSystem::GetClientLogger()->PrintMsg(PhxEngine::LogLevel::Trace, msg, __VA_ARGS__)
#define LOG_INFO(msg, ...)	::PhxEngine::LogSystem::GetClientLogger()->PrintMsg(PhxEngine::LogLevel::Info, msg, __VA_ARGS__)
#define LOG_WARN(msg, ...)	::PhxEngine::LogSystem::GetClientLogger()->PrintMsg(PhxEngine::LogLevel::Warn, msg, __VA_ARGS__)
#define LOG_ERROR(msg, ...)	::PhxEngine::LogSystem::GetClientLogger()->PrintMsg(PhxEngine::LogLevel::Error, msg, __VA_ARGS__)
#define LOG_FATAL(msg, ...)	::PhxEngine::LogSystem::GetClientLogger()->PrintMsg(PhxEngine::LogLevel::Fatal, msg, __VA_ARGS__)


// Strip out for distribution builds.