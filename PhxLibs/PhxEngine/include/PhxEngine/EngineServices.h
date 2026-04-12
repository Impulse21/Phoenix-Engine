#pragma once

// -- Forward Declares ---
namespace phx
{
    class IIoQueue;
    class IRootFileSystem;
}

namespace phx
{
    struct EngineServices
    {
        // -- Depericated ---
        IRootFileSystem* root_file_system;
        
        // -- Depericated ---
        IIoQueue* io_queue;
    };
}