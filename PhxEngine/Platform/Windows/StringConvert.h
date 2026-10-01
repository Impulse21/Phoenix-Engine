#pragma once

#include <string>

#include <stringapiset.h>

namespace phx::platform
{
	inline void StringConvert(std::string const& from, std::wstring& to)
	{
		int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, nullptr, 0);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
		}
	}

	inline void StringConvert(std::wstring const& from, std::string& to)
	{
		int num = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, &to[0], num, nullptr, nullptr);
		}
	}
}