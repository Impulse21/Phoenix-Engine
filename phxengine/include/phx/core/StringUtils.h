#pragma once

#include "phx/core/PlatformDetection.h"
#include <string>


namespace phx
{
	inline void StringConvert(std::string const& from, std::wstring& to)
	{
#ifdef PHX_PLATFORM_WINDOWS
		int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, NULL, 0);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
		}
#else
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		to = cv.from_bytes(from);
#endif // _WIN32

	}

	inline void StringConvert(std::wstring const& from, std::string& to)
	{
#ifdef PHX_PLATFORM_WINDOWS
		int num = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, &to[0], num, NULL, NULL);
		}
#else
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		to = cv.to_bytes(from);
#endif // _WIN32
	}

}