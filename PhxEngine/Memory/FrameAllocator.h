#pragma once

#include <PhxEngine/Memory/LinearAllocator.h>

namespace phx
{
    // A thread's cursor into the per-frame arena (see FrameArena).
    // Memory is valid until Memory::EndFrame() of the frame it was allocated
    // in, regardless of which thread reads it. Get the calling thread's with
    // Memory::GetFrameAlloc(); never hand one to another thread.
    class FrameAllocator : public LinearAllocator
    {
    public:
        PHX_NO_COPY_NO_MOVE(FrameAllocator);

        FrameAllocator() = default;
    };


    template<typename T>
    using FramePtr = T*;
}
