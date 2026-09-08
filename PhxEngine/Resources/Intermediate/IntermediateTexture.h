#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/RHI/RHITypes.h>

namespace phx::resources
{
    struct IntermediateTexture
    {
        PHX_MOVE_ONLY(IntermediateTexture);

        MemoryBuffer pixel_data         = {};

        uint32_t width                  = 0;
        uint32_t height                 = 0;

        uint32_t depth                  = 1; 
        uint32_t array_layers           = 1;

        std::vector<size_t> mip_offsets = {};

        rhi::Format format = rhi::Format::UNKNOWN;

        bool IsValid() const { return pixel_data.Data() != nullptr; }
        uint32_t GetMipCount() const { return static_cast<uint32_t>(mip_offsets.size()); }
    };
}