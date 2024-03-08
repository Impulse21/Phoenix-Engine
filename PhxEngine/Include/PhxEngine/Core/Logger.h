#pragma once

#include <string>
#include <functional>

namespace PhxEngine
{
	namespace Logger
	{
		enum class LogLevel
		{
			None,
			Info,
			Warning,
			Error,
		};

		struct LogEntry
		{
			LogLevel Level = LogLevel::None;
			std::string Msg;
		};

		void Startup();
		void Shutdown();
		
		using LogCallbackFn = std::function<void(const LogEntry&)>;
		void RegisterLogCallback(LogCallbackFn const& logTarget);

		void PostMsg(std::string_view categroy, LogLevel logLEvel, std::string_view msg, ...);
	}
}

constexpr std::string_view CatCore = "Core";
constexpr std::string_view CatApp = "App";

#define PHX_LOG_CORE_INFO(...)	PhxEngine::Logger::PostMsg(CatCore, PhxEngine::Logger::LogLevel::Info, __VA_ARGS__)
#define PHX_LOG_CORE_WARN(...)	PhxEngine::Logger::PostMsg(CatCore, PhxEngine::Logger::LogLevel::Warning, __VA_ARGS__)
#define PHX_LOG_CORE_ERROR(...) PhxEngine::Logger::PostMsg(CatCore, PhxEngine::Logger::LogLevel::Error, __VA_ARGS__)
										 
#define PHX_LOG_INFO(...)		PhxEngine::Logger::PostMsg(CatApp, PhxEngine::Logger::LogLevel::Info, __VA_ARGS__)
#define PHX_LOG_WARN(...)		PhxEngine::Logger::PostMsg(CatApp, PhxEngine::Logger::LogLevel::Warning, __VA_ARGS__)
#define PHX_LOG_ERROR(...)		PhxEngine::Logger::PostMsg(CatApp, PhxEngine::Logger::LogLevel::Error, __VA_ARGS__)