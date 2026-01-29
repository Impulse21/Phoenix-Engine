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
		PHX_CORE_WARN("Failed to retrieve platform file attributes: {0}", norm_physical_path);
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

phx::Result<PlatformFileHandle> phx::platform::windows::WindowsPlatformWrapperImpl::PlatformOpenFile(const std::string& os_path, const char* mode)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, os_path.c_str(), mode);
	if (err != 0)
		return make_unexpected(0ull);

	return PlatformFileHandle{ fp };
}

void phx::platform::windows::WindowsPlatformWrapperImpl::PlatformCloseFile(PlatformFileHandle handle)
{
	if (handle.IsValid()) 
	{
		fclose(handle.As<FILE>());
	}
}

bool phx::platform::windows::WindowsPlatformWrapperImpl::PlatformSeekFile(PlatformFileHandle handle, int64_t offset, FileSeekOrigin origin)
{
	if (!handle.IsValid()) 
		return false;

	int whence = 0;
	switch (origin)
	{
	case FileSeekOrigin::Begin:
		whence = SEEK_SET;
		break;
	case FileSeekOrigin::Current:
		whence = SEEK_CUR;
		break;
	case FileSeekOrigin::End:
		whence = SEEK_END;
		break;
	};

	return _fseeki64(handle.As<FILE>(), offset, whence) == 0;
}

void phx::platform::windows::WindowsPlatformWrapperImpl::PlatformWriteFile(PlatformFileHandle handle, const char* buffer, size_t size_to_write)
{
	if (!handle.IsValid() || !buffer || size_to_write == 0)
		return;

	fwrite(buffer, 1, size_to_write, handle.As<FILE>());
}

size_t phx::platform::windows::WindowsPlatformWrapperImpl::PlatformReadFile(PlatformFileHandle handle, void* buffer, size_t size_to_read)
{
	if (!handle.IsValid() || !buffer || size_to_read == 0) 
		return 0;

	return fread(buffer, 1, size_to_read, handle.As<FILE>());
}

phx::Result<phx::Span<char>> phx::platform::windows::WindowsPlatformWrapperImpl::PlatformGetEmbeddedResource(std::string const& resource_name)
{
	std::wstring w_resource_name;
	StringConvert(resource_name, w_resource_name);

	HRSRC hRes = FindResource(nullptr, w_resource_name.c_str(), RT_RCDATA);
	if (hRes == nullptr)
		return make_unexpected(~0ull);

	HGLOBAL hGlob = LoadResource(nullptr, hRes);
	if (hGlob == nullptr)
		return make_unexpected(~0ull);

	const char* data = static_cast<const char*>(LockResource(hGlob));
	if (data == nullptr)
		return make_unexpected(~0ull);

	DWORD size = SizeofResource(nullptr, hRes);
	return phx::Span<char>(data, static_cast<size_t>(size));
}
