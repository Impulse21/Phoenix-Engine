#include <PhxEngine/Platform/Thread.h>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>


using namespace phx::platform;

void Thread::SetThreadName(std::thread& thread, const char* name)
{
	HANDLE handle = thread.native_handle();

	std::wstring nameW;
	StringConvert(name, nameW);

	HRESULT hr = SetThreadDescription(handle, nameW.c_str());
	PHX_ASSERT(SUCCEEDED(hr));
}

void Thread::SetThreadAffinity(std::thread& thread, int affinity)
{
	HANDLE handle = thread.native_handle();

	DWORD_PTR affinity_mask = 1ull << affinity;
	DWORD_PTR result = SetThreadAffinityMask(handle, affinity_mask);
	PHX_ASSERT(result > 0);
}

void Thread::SetThreadPriority(std::thread& thread, Priority prio)
{
	 static constexpr int kPriorityMap[] = 
	 {
        THREAD_PRIORITY_ABOVE_NORMAL,  // High
        THREAD_PRIORITY_NORMAL,        // Normal
        THREAD_PRIORITY_LOWEST,        // Low
    };

	HANDLE handle = thread.native_handle();

    const int os_priority = kPriorityMap[(int)prio];
	PHX_ASSERT(::SetThreadPriority(handle, os_priority) != 0);
}