#pragma once

#ifdef _WIN32

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

namespace PhxEngine::Core::Platform
{
#ifdef _WIN32
#ifdef PLATFORM_UWP
using WindowHandle = const winrt::Windows::UI::Core::CoreWindow*;
#else
using WindowHandle = HWND;
#endif // PLATFORM_UWP
#endif
}