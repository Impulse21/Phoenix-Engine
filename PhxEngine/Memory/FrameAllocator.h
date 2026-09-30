#pragma once

#include <PhxEngine/Memory/LinearAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

namespace phx
{
    class FrameAllocator : public LinearAllocator 
    {
    public:
        PHX_NO_COPY_NO_MOVE(FrameAllocator);

        FrameAllocator() = default;
    };
}