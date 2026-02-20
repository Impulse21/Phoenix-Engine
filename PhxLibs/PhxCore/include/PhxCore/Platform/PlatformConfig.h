#pragma once

// -- Platform Detections (Mostly driven by cmake now now adays) ---
#ifdef _WIN32
    /* Windows x64/x86 */
#ifdef _WIN64
    /* Windows x64  */
#ifndef PHX_PLATFORM_WINDOWS
#define PHX_PLATFORM_WINDOWS
#endif
#else
    /* Windows x86 */
#error "x86 Builds are not supported!"
#endif
#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
/* TARGET_OS_MAC exists on all the platforms
 * so we must check all of them (in this order)
 * to ensure that we're running on MAC
 * and not some other Apple platform */
#if TARGET_IPHONE_SIMULATOR == 1
#error "IOS simulator is not supported!"
#elif TARGET_OS_IPHONE == 1
#define PHX_PLATFORM_IOS
#error "IOS is not supported!"
#elif TARGET_OS_MAC == 1
#define PHX_PLATFORM_MACOS
#error "MacOS is not supported!"
#else
#error "Unknown Apple platform!"
#endif
 /* We also have to check __ANDROID__ before __linux__
  * since android is based on the linux kernel
  * it has __linux__ defined */
#elif defined(__ANDROID__)

#define PHX_PLATFORM_ANDROID
#error "Android is not supported!"

#elif defined(__linux__)

#ifndef PHX_PLATFORM_LINUX
#define PHX_PLATFORM_LINUX
#endif

#else
    /* Unknown compiler/platform */
#error "Unknown platform!"
#endif // End of platform detection

// -- Platform Definitions ---
#if defined(PHX_PLATFORM_WINDOWS)
    namespace phx
    {
        // This is the type your engine code will refer to as phx::rhi::CommandBuffer
        using window_type = HWND;
    }
    
#elif defined(PHX_PLATFORM_LINUX)

    namespace phx
    {
        // This is the type your engine code will refer to as phx::rhi::CommandBuffer
        // using window_type = wl_surface;
        using window_type = void*; // TODO: Need to learn what this needs to be.
    }

#else

#error "Unsupported platform. Currently only support windows and linux."

#endif