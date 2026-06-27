#include "Thread.h"

#include <thread>

namespace
{
    std::thread::id     g_mainThreadId;
    thread_local bool   g_isMainThread = false;
}

namespace phx
{
    void Thread::Initialize()
    {
        g_mainThreadId = std::this_thread::get_id();
        g_isMainThread = true;
    }

    bool Thread::IsMainThread()
    {
        return g_isMainThread;
    }
}