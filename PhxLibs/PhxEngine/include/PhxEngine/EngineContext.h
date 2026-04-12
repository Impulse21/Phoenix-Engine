#pragma once

#include <PhxCore/Handle.h>
#include <PhxCore/Platform/PlatformWindow.h>

// -- Forward Declares ---
namespace phx
{
    class IIoQueue;
}

namespace phx
{
    struct EngineContext
    {
        // -- Depericated ---
        IIoQueue* io_queue;

        WindowHandle window_handle;
    };
}