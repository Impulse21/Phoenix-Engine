#pragma once

#ifdef _WIN32

#define PHX_PLATFORM_WINDOWS_DESKTOP

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


	struct WindowProperties
	{
		int Width = 0;
		int Height = 0;
		float Dpi = 0.0f;
	};

	inline void GetWindowProperties(WindowHandle windowHandle, WindowProperties& outProperties)
	{
#ifdef PHX_PLATFORM_WINDOWS_DESKTOP
		outProperties.Dpi = static_cast<float>(GetDpiForWindow(windowHandle));
		RECT rect = {};
		GetClientRect(windowHandle, &rect);
		outProperties.Width = static_cast<int>(rect.right - rect.left);
		outProperties.Height = static_cast<int>(rect.bottom - rect.top);
#endif
	}

	inline bool VerifyWindowsVersion(uint32_t majorVersion, uint32_t minorVersion, uint32_t buildNumber)
	{
#ifdef PHX_PLATFORM_WINDOWS_DESKTOP
		OSVERSIONINFOEX Version;
		Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		Version.dwMajorVersion = majorVersion;
		Version.dwMinorVersion = minorVersion;
		Version.dwBuildNumber = buildNumber;

		ULONGLONG ConditionMask = 0;
		ConditionMask = VerSetConditionMask(ConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
		ConditionMask = VerSetConditionMask(ConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
		ConditionMask = VerSetConditionMask(ConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

		return !!VerifyVersionInfo(&Version, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, ConditionMask);
#else
		return false;
#endif

	}
}