#include <PhxEngine/Core/Threading.h>

#include <stdint.h>
#include <thread>

namespace
{
    using ThreadID = std::thread::id;
    ThreadID mainThreadId;

    ThreadID GetCurrentThreadID()
    {
        return std::this_thread::get_id();
    }

}
void PhxEngine::Core::Threading::SetMainThread()
{
    mainThreadId = GetCurrentThreadID();
}

bool PhxEngine::Core::Threading::IsMainThread()
{
    return GetCurrentThreadID() == mainThreadId;
}
