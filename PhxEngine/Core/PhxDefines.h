#pragma once

// ── Compiler guard ────────────────────────────────────────────────────────────
// We enforce Clang in CMake — this is a safety net
#if !defined(__clang__)
    #error "PHX requires Clang. Check your toolchain file."
#endif

// ── Platform validation ───────────────────────────────────────────────────────
// PHX_PLATFORM_WINDOWS / PHX_PLATFORM_LINUX come from CMake
#if defined(PHX_PLATFORM_WINDOWS)
    #define PHX_PLATFORM_NAME "Windows"
#elif defined(PHX_PLATFORM_LINUX)
    #define PHX_PLATFORM_NAME "Linux"
#else
    #error "PHX: Unknown platform — missing CMake define"
#endif

// ── Architecture ──────────────────────────────────────────────────────────────
#if defined(__x86_64__)
    #define PHX_ARCH_X64    1
#elif defined(__aarch64__)
    #define PHX_ARCH_ARM64  1
#else
    #error "PHX: Unknown architecture"
#endif

// ── Config validation ─────────────────────────────────────────────────────────
// PHX_DEBUG / PHX_RELEASE come from CMake — catch misconfigured builds early
#if !defined(PHX_DEBUG) && !defined(PHX_RELEASE)
    #error "PHX: No config defined — ensure CMake sets PHX_DEBUG or PHX_RELEASE"
#endif

// ── Primitive types ───────────────────────────────────────────────────────────
#include <cstdint>
#include <cstddef>

using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;

using i8    = int8_t;
using i16   = int16_t;
using i32   = int32_t;
using i64   = int64_t;

using f32   = float;
using f64   = double;

using usize = size_t;
using isize = ptrdiff_t;
using b8    = uint8_t;  // explicit-width bool for structs

// ── Compiler hints ────────────────────────────────────────────────────────────
// All Clang — no MSVC fallbacks needed

#define PHX_INLINE              __attribute__((always_inline)) inline
#define PHX_NOINLINE            __attribute__((noinline))
#define PHX_LIKELY(x)           __builtin_expect(!!(x), 1)
#define PHX_UNLIKELY(x)         __builtin_expect(!!(x), 0)
#define PHX_UNREACHABLE()       __builtin_unreachable()
#define PHX_DEBUGBREAK()        __builtin_debugtrap()
#define PHX_UNUSED(x)           (void)(x)
#define PHX_ALIGN(n)            __attribute__((aligned(n)))
#define PHX_PACKED              __attribute__((packed))
#define PHX_NODISCARD           [[nodiscard]]
#define PHX_FALLTHROUGH         [[fallthrough]]
#define PHX_CACHELINE           64
#define PHX_CACHELINE_ALIGN     PHX_ALIGN(PHX_CACHELINE)

// ── Utility macros ────────────────────────────────────────────────────────────
#define PHX_ARRAY_COUNT(arr)    (sizeof(arr) / sizeof((arr)[0]))
#define PHX_OFFSETOF(T, m)      offsetof(T, m)
#define PHX_BIT(n) (1u << (n))
#define PHX_BIT64(n) (1ull << (n))

[[nodiscard]] constexpr size_t PhxBytesToKB(size_t bytes) noexcept
{
  return bytes >> 10;
}
[[nodiscard]] constexpr size_t PhxBytesToMB(size_t bytes) noexcept
{
  return bytes >> 20;
}
[[nodiscard]] constexpr size_t PhxBytesToGB(size_t bytes) noexcept
{
  return bytes >> 30;
}

[[nodiscard]] constexpr size_t PhxKB(size_t size) noexcept
{
  return size << 10;
}
[[nodiscard]] constexpr size_t PhxMB(size_t size) noexcept
{
  return size << 20;
}
[[nodiscard]] constexpr size_t PhxGB(size_t size) noexcept
{
  return size << 30;
}

constexpr size_t operator"" _KB(unsigned long long bytes)
{
  return bytes << 10;
}
constexpr size_t operator"" _MB(unsigned long long bytes)
{
  return bytes << 20;
}
constexpr size_t operator"" _GB(unsigned long long bytes)
{
  return bytes << 30;
}

// ── Assert ────────────────────────────────────────────────────────────────────
// Intentionally minimal here — no logging dependency.
// Full PHX_ASSERT with log output lives in Engine/Core/Assert.h
// and is included explicitly where needed.

#if defined(PHX_DEBUG)
    #define PHX_ASSERT_BASIC(cond)  \
        do {                        \
            if (PHX_UNLIKELY(!(cond))) { PHX_DEBUGBREAK(); } \
        } while(0)
#else
    #define PHX_ASSERT_BASIC(cond)  PHX_UNUSED(cond)
#endif

// ── Non-copyable / non-movable helpers ────────────────────────────────────────
#define PHX_NO_COPY(T)                      \
    T(const T&)            = delete;        \
    T& operator=(const T&) = delete

#define PHX_NO_MOVE(T)                      \
    T(T&&)            = delete;             \
    T& operator=(T&&) = delete

#define PHX_NO_COPY_NO_MOVE(T)  \
    PHX_NO_COPY(T);             \
    PHX_NO_MOVE(T)