//
// pch.h
// Header for standard system include files.
//

#pragma once

#if defined(PHX_PLATFORM_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
#define VK_USE_PLATFORM_WIN32_KHR

#elif defined(PHX_PLATFORM_LINUX)

#define VK_USE_PLATFORM_WAYLAND_KHR
#endif


#include <PhxCore/Base.h>

#include <PhxCore/Log.h>
#include <PhxCore/Profiler.h>

#ifdef PHX_RHI_VULKAN
#define VK_NO_PROTOTYPES

#define USE_BUFFER_ADDRESS true

#define vulkan_check(call) [&]() { VkResult res = call; PHX_CORE_ASSERT(res >= VK_SUCCESS); return res; }()
#endif