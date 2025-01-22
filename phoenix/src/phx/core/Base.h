#pragma once

#include "PlatformDetection.h"
#include <memory>

#ifdef PHX_DEBUG
#if defined(PHX_PLATFORM_WINDOWS)
#define PHX_DEBUGBREAK() __debugbreak()
#elif defined(PHX_PLATFORM_LINUX)
#include <signal.h>
#define PHX_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#define PHX_ENABLE_ASSERTS
#else
#define PHX_DEBUGBREAK()
#endif

#define PHX_EXPAND_MACRO(x) x
#define PHX_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define PHX_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

#include "phx/core/Log.h"
#include "phx/core/Assert.h"
