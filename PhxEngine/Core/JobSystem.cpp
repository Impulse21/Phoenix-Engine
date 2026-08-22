#include "JobSystem.h"


using namespace phx;

namespace phx
{
    struct ThreadPool
    {

    };
}

namespace
{
    IHeapAllocator* g_allocator = nullptr;
    ThreadPoolHandle g_core_pool = nullptr;
}

namespace phx::Jobs
{
    void Initialize(IHeapAllocator* allocator)
    {
        g_allocator = allocator;


    }

    void Shutdown()
    {
        // Implementation for shutting down the job system
    }

    void BeginFrame()
    {
        // Implementation for beginning a new frame in the job system
    }

    ThreadPoolHandle CreateThreadPool(const ThreadPoolDescriptor& desc)
    {
        // Implementation for creating a new thread pool
        return nullptr; // Placeholder return value
    }

    void DestroyThreadPool(ThreadPoolHandle pool)
    {
        // Implementation for destroying a thread pool
    }
}