#include "PhxCore/PhxCore_pch.h"

#include "WindowsPlatformWrapper.h"
#include <PhxCore/StringUtils.h>

#define PATH_MAX MAX_PATH

using namespace phx::platform;
using namespace phx::platform::windows;

namespace
{
	inline Timestamp FileTimeToTimestamp(const FILETIME& ft)
	{
		ULARGE_INTEGER uli;
		uli.LowPart = ft.dwLowDateTime;
		uli.HighPart = ft.dwHighDateTime;

		// FILETIME is in 100-nanosecond intervals since January 1, 1601 (UTC).
		constexpr int64_t c_epoch_diff_seconds = 11644473600LL;

		uint64_t intervals = uli.QuadPart;

		int64_t seconds_since_1601 = static_cast<int64_t>(intervals / 10000000ULL);
		int64_t nanoseconds_remainder = static_cast<int64_t>((intervals % 10000000ULL) * 100);

		int64_t seconds_since_1970 = seconds_since_1601 - c_epoch_diff_seconds;

		auto total_ns = std::chrono::seconds(seconds_since_1970) + std::chrono::nanoseconds(nanoseconds_remainder);

		// Explicitly cast to system_clock::duration
		return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(total_ns));
	}
}

void* WindowsPlatformWrapperImpl::PlatformVirtualMemReserve(size_t reserveSize)
{
	return VirtualAlloc(nullptr, reserveSize, MEM_RESERVE, PAGE_READWRITE);
}

void WindowsPlatformWrapperImpl::PlatformVirtualMemCommit(void* ptr, size_t commitSize)
{
	VirtualAlloc(ptr, commitSize, MEM_COMMIT, PAGE_READWRITE);
}

bool WindowsPlatformWrapperImpl::PlatformVirtualMemFree(void* ptr)
{
	return VirtualFree(ptr, 0, MEM_RELEASE);
}

phx::Result<std::string> phx::platform::windows::WindowsPlatformWrapperImpl::PlatformGetExectuablePath()
{
	char path[PATH_MAX] = { 0 };
	if (GetModuleFileNameA(nullptr, path, PATH_MAX) == 0)
		return make_unexpected(~0ull);

	return path;
}

phx::Result<PlatformFileAttributes>  WindowsPlatformWrapperImpl::PlatformGetFileAttributes(std::string const& norm_physical_path)
{
	std::wstring wide_os_path;
	StringConvert(norm_physical_path, wide_os_path);

	WIN32_FILE_ATTRIBUTE_DATA win_file_attributes;
	if (!GetFileAttributesExW(wide_os_path.c_str(), GetFileExInfoStandard, &win_file_attributes))
	{
		PHX_CORE_ERROR("Failed to retrieve platform file attributes: {0}", norm_physical_path);
		return make_unexpected(~0ull);
	}

	PlatformFileAttributes attrs;
	attrs.type = win_file_attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
		? PlatformFileType::Directory
		: PlatformFileType::File;

	ULARGE_INTEGER file_size;
	file_size.HighPart = win_file_attributes.nFileSizeHigh;
	file_size.LowPart = win_file_attributes.nFileSizeLow;
	attrs.size = file_size.QuadPart;

	attrs.creation_time = FileTimeToTimestamp(win_file_attributes.ftCreationTime);
	attrs.last_access_time = FileTimeToTimestamp(win_file_attributes.ftLastAccessTime);
	attrs.last_write_time = FileTimeToTimestamp(win_file_attributes.ftLastWriteTime);

	attrs.is_read_only = (win_file_attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
	attrs.is_hidden = (win_file_attributes.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
	return attrs;
}
