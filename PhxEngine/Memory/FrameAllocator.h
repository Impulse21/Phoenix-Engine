#pragma once

#include <PhxEngine/Memory/LinearAllocator.h>

namespace phx
{
    class FrameAllocator : public LinearAllocator 
    {
    public:
        PHX_NO_COPY_NO_MOVE(FrameAllocator);

        FrameAllocator() = default;
    };
}