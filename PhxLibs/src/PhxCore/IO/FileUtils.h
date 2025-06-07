#pragma once

#include <string>
#include <filesystem>

#ifndef PHX_PLATFORM_WINDOWS
#include <unistd.h>
#include <cstdio>
#include <climits>
#else
#define PATH_MAX MAX_PATH
#endif // _WIN32

namespace phx
{

	inline std::string GetWorkingDirectory()
	{
		return std::filesystem::current_path().generic_string();
	}

	inline std::string GetDirectoryWithExecutable()
	{
		char path[PATH_MAX] = { 0 };
#ifdef PHX_PLATFORM_WINDOWS
		if (GetModuleFileNameA(nullptr, path, PATH_MAX) == 0)
			return "";
#else // _WIN32
		// /proc/self/exe is mostly linux-only, but can't hurt to try it elsewhere
		if (readlink("/proc/self/exe", path, std::size(path)) <= 0)
		{
			// portable but assumes executable dir == cwd
			if (!getcwd(path, std::size(path)))
				return ""; // failure
		}
#endif // PHX_PLATFORM_WINDOWS

		std::filesystem::path result = path;
		result = result.parent_path();

		return result.generic_string();
	}

	inline std::string GetFileNameWithoutExt(std::string const& path)
	{
		return std::filesystem::path(path).stem().generic_string();
	}
	inline std::string GetFileExt(std::string const& path)
	{
		return std::filesystem::path(path).extension().generic_string();
	}

	inline std::string JoinPaths(const std::string& p1, const std::string& p2)
	{
		if (p1.empty())
			return p2;

		if (p2.empty())
			return p1;

		char sep = '/'; // Normalize to one separator type
#ifdef PHX_PLATFORM_WINDOWS
		// sep = '\\'; // Or keep '/' and let Windows handle it
#endif

		std::string result = p1;
		if (result.back() == '/' || result.back() == '\\')
		{
			if (p2.front() == '/' || p2.front() == '\\')
			{
				result += p2.substr(1);
			}
			else {
				result += p2;
			}
		}
		else
		{
			if (p2.front() == '/' || p2.front() == '\\')
			{
				result += p2;
			}
			else {
				result += sep;
				result += p2;
			}
		}

		// Replace all '\\' with '/' for internal consistency if desired
		std::replace(result.begin(), result.end(), '\\', '/');
		return result;
	}
}