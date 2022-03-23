#pragma once

#include <string>
#include <iostream>
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

		void PrintMsg(LogLevel level, const char* msg);

		template<typename ... Args>
		void PrintMsg(LogLevel level, std::string const& fmt, Args&&... args)
		{
			std::string msg = StringFormat(fmt, Convert(std::forward<Args>(args))...);
			PrintMsg(level, msg.c_str());
		}

	private:
		/**
		 * Convert all std::strings to const char* using constexpr if (C++17)
		 */
		template<typename T>
		auto Convert(T&& t) 
		{
			if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value) 
			{
				return std::forward<T>(t).c_str();
			}
			else 
			{
				return std::forward<T>(t);
			}
		}

		template<typename ... Args>
		std::string StringFormat(std::string const& format, Args ... args)
		{
			int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
			if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
			auto size = static_cast<size_t>(size_s);
			std::unique_ptr<char[]> buf(new char[size]);
			std::snprintf(buf.get(), size, format.c_str(), args ...);
			return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
		}

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