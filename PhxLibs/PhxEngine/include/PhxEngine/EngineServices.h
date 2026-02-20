#pragma once

// -- Forward Declares ---
namespace phx
{
    class IIoQueue;
    class IVirtualFileSystem;
}

namespace phx
{
    struct EngineServices
    {
        // -- Depericated ---
        IVirtualFileSystem* virtual_file_system;
        
        // -- Depericated ---
        IIoQueue* io_queue;
        
    };
}