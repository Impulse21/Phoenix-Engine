#pragma once

#include <string>
#include <memory>
#include <string_view>
#include <PhxEngine/Core/Span.h>
#include <functional>

namespace PhxEngine::Core
{
	namespace Log
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
			Log::LogLevel Level = Log::LogLevel::None;
			std::string Msg;
		};

		void Initialize();
		void Finialize();

		using LogCallbackFn = std::function<void(const LogEntry&)>;
		void RegisterLogCallback(LogCallbackFn const& logTarget);

		void PostMsg(std::string_view categroy, LogLevel logLEvel, std::string_view msg, ...);
	}
}

constexpr std::string_view CatCore = "Core";
constexpr std::string_view CatApp = "App";
constexpr std::string_view CatRHI = "RHI";

// -- Core log macros ----
#define LOG_CORE_INFO(...)		PhxEngine::Core::Log::PostMsg(CatCore, PhxEngine::Core::Log::LogLevel::Info, __VA_ARGS__)
#define LOG_CORE_WARN(...)		PhxEngine::Core::Log::PostMsg(CatCore, PhxEngine::Core::Log::LogLevel::Warning, __VA_ARGS__)
#define LOG_CORE_ERROR(...)		PhxEngine::Core::Log::PostMsg(CatCore, PhxEngine::Core::Log::LogLevel::Error, __VA_ARGS__)

#define LOG_INFO(...)			PhxEngine::Core::Log::PostMsg(CatApp, PhxEngine::Core::Log::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)			PhxEngine::Core::Log::PostMsg(CatApp, PhxEngine::Core::Log::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...)			PhxEngine::Core::Log::PostMsg(CatApp, PhxEngine::Core::Log::LogLevel::Error, __VA_ARGS__)

#define LOG_RHI_INFO(...)		PhxEngine::Core::Log::PostMsg(CatRHI, PhxEngine::Core::Log::LogLevel::Info, __VA_ARGS__)
#define LOG_RHI_WARN(...)		PhxEngine::Core::Log::PostMsg(CatRHI, PhxEngine::Core::Log::LogLevel::Warning, __VA_ARGS__)
#define LOG_RHI_ERROR(...)		PhxEngine::Core::Log::PostMsg(CatRHI, PhxEngine::Core::Log::LogLevel::Error, __VA_ARGS__)

#define PHX_LOG_CORE_INFO(...)	PhxEngine::Core::Log::PostMsg(CatCore, PhxEngine::Core::Log::LogLevel::Info, __VA_ARGS__)
#define PHX_LOG_CORE_WARN(...)	PhxEngine::Core::Log::PostMsg(CatCore, PhxEngine::Core::Log::LogLevel::Warning, __VA_ARGS__)
#define PHX_LOG_CORE_ERROR(...) PhxEngine::Core::Log::PostMsg(CatCore, PhxEngine::Core::Log::LogLevel::Error, __VA_ARGS__)

#define PHX_LOG_INFO(...)		PhxEngine::Core::Log::PostMsg(CatApp, PhxEngine::Core::Log::LogLevel::Info, __VA_ARGS__)
#define PHX_LOG_WARN(...)		PhxEngine::Core::Log::PostMsg(CatApp, PhxEngine::Core::Log::LogLevel::Warning, __VA_ARGS__)
#define PHX_LOG_ERROR(...)		PhxEngine::Core::Log::PostMsg(CatApp, PhxEngine::Core::Log::LogLevel::Error, __VA_ARGS__)