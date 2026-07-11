#pragma once

#include <PhxEngine/Memory/LinearAllocator.h>

namespace phx
{
    class ScratchAllocator : public LinearAllocator 
    {
    public:
        PHX_NO_COPY_NO_MOVE(ScratchAllocator);
        ScratchAllocator() = default;
    };
}