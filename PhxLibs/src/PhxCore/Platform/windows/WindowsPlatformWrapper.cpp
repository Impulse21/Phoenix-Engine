#include "PhxCore/PhxCore_pch.h"

#include "WindowsPlatformWrapper.h"
#include <PhxCore/StringUtils.h>

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
		// std::chrono::system_clock epoch is typically January 1, 1970 (UTC).
		// The difference is 11644473600 seconds.
		constexpr int64_t c_epoch_diff_seconds = 11644473600LL; // Use signed for subtraction
		uint64_t intervals = uli.QuadPart;

		// Convert intervals to seconds and nanoseconds relative to FILETIME epoch
		int64_t seconds_since_1601 = static_cast<int64_t>(intervals / 10000000ULL);
		int64_t nanoseconds_remainder = static_cast<int64_t>((intervals % 10000000ULL) * 100);

		// Adjust seconds to be relative to system_clock epoch (1970)
		int64_t seconds_since_1970 = seconds_since_1601 - c_epoch_diff_seconds;

		// Construct time_point
		// Note: std::chrono::system_clock::from_time_t takes time_t (usually seconds since epoch)
		// This conversion might be slightly different based on system_clock's exact epoch guarantee,
		// but for practical purposes this approach is common.
		// More robust might involve direct duration calculations from a known epoch.
		std::chrono::seconds s(seconds_since_1970);
		std::chrono::nanoseconds ns(nanoseconds_remainder);
		return std::chrono::system_clock::time_point(s + ns);
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

phx::Result<PlatformFileAttributes>  WindowsPlatformWrapperImpl::PlatformGetFileAttributes(std::string const& norm_physical_path)
{
	std::wstring wide_os_path;
	StringConvert(norm_physical_path, wide_os_path);

	WIN32_FILE_ATTRIBUTE_DATA win_file_attributes;
	if (!GetFileAttributesExW(wide_os_path.c_str(), GetFileExInfoStandard, &win_file_attributes))
	{
		PHX_CORE_ERROR("Failed to retrieve platform file attributes: {0}", norm_physical_path);
		return "Failed to retrieve platform file attributes: " + norm_physical_path;
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
