#pragma once

#include "phxPlatformDetection.h"

#if defined(PHX_PLATFORM_WINDOWS)

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <SDKDDKVer.h>
#include <windows.h>
#include <tchar.h>

#endif

namespace phx::platform
{

#ifdef PHX_PLATFORM_WINDOWS
	using window_type = HWND;
#else
	using window_type = void*;
#endif // _WIN32
}