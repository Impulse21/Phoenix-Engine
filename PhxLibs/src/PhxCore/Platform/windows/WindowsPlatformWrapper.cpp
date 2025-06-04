#include "PhxCore/PhxCore_pch.h"

#include "WindowsPlatformWrapper.h"
#include <PhxCore/StringUtils.h>

using namespace phx::platform::windows;


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

phx::platform::FileAttributes WindowsPlatformWrapperImpl::PlatformGetFileAttributes(std::string const& norm_physical_path)
{
	std::wstring wide_norm_physical_path;
	StringConvert(norm_physical_path, wide_norm_physical_path);

	DWORD attrib = GetFileAttributesW(wide_norm_physical_path.c_str());

	return static_cast<FileAttributes>(attrib);
}
