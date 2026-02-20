#pragma once

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

#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define PHX_FORCE_INLINE __forceinline

#elif defined(__GNUC__) || defined(__clang__)
// GCC and Clang Compilers
#define PHX_FORCE_INLINE [[gnu::always_inline]] inline
// You might also see the older __attribute__((always_inline))

#else
// Fallback for unknown compilers
#define PHX_FORCE_INLINE inline
#endif

struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

#include <PhxCore/Log.h>
#include "PhxCore/Assert.h"


#define PhxKB(size)                 (size * 1024)
#define PhxMB(size)                 (size * 1024 * 1024)
#define PhxGB(size)                 (size * 1024 * 1024 * 1024)

#define PhxToKB(x)					((size_t) (x) >> 10)
#define PhxToMB(x)					((size_t) (x) >> 20)
#define PhxToGB(x)					((size_t) (x) >> 30)

constexpr inline unsigned long long operator ""_KiB(unsigned long long value)
{
	return value << 10;
}

constexpr inline unsigned long long operator ""_MiB(unsigned long long value)
{
	return value << 20;
}

constexpr inline unsigned long long operator ""_GiB(unsigned long long value)
{
	return value << 30;
}

#define PHX_MAX_PATH			MAX_PATH
#define ALIGNAS(x)             __declspec(align(x))
#define DEFINE_ALIGNED(def, a) __declspec(align(a)) def
#define THREAD_LOCAL           __declspec(thread)

#define CompileTimeAssertSize(def, size) static_assert(sizeof(def) == size, "Definition #def must be #size bytes")

template<typename T, typename M>
constexpr size_t phx_offsetof(M T::* member)
{
	return reinterpret_cast<size_t>(&(((T*)0)->*member));
}
