#pragma once

#include "Core/phxLog.h"

#define PHX_UNUSED [[maybe_unused]]


struct NonMoveable
{
    NonMoveable() = default;
    NonMoveable(NonMoveable&&) = delete;
    NonMoveable& operator = (NonMoveable&&) = delete;
};

struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

namespace phx
{

	inline void StringConvert(std::string const& from, std::wstring& to)
	{
#ifdef _WIN32
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
#ifdef _WIN32
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

	inline std::string StringReplace(std::string_view str, std::string_view oldSubstr, std::string_view newSubStr)
	{
		std::string retVal = std::string(str);
		// Find the position of the substring
		size_t pos = str.find(oldSubstr);
		if (pos != std::string::npos)
		{
			retVal.replace(pos, oldSubstr.size(), newSubStr);
		}

		return retVal;

	}
}