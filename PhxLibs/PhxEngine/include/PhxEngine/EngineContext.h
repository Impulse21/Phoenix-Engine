#pragma once

#include <PhxCore/Handle.h>

// -- Forward Declares ---
namespace phx
{
    class IIoQueue;
    class IVirtualFileSystem;

    PHX_DEFINE_OPAQUE_HANDLE(Window)
}

namespace phx
{
    struct EngineContext
    {
        // -- Depericated ---
        IVirtualFileSystem* virtual_file_system;
        
        // -- Depericated ---
        IIoQueue* io_queue;

        Window window;
    };
}