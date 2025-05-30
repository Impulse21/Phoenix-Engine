#include "PhxCore/PhxCore_pch.h"

#include "WindowsPlatformWrapper.h"

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