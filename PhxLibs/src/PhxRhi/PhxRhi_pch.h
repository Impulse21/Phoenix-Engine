//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

// Use the C++ standard templated min/max
#ifndef NOMINMAX
#define NOMINMAX
#endif
// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "PhxCore/Base.h"

#include "PhxCore/Log.h"
#include <PhxCore/Profiler.h>

#ifdef PHX_RHI_VULKAN
#define VK_NO_PROTOTYPES

#define USE_BUFFER_ADDRESS true

#define vulkan_check(call) [&]() { VkResult res = call; PHX_CORE_ASSERT(res >= VK_SUCCESS); return res; }()
#endif