#pragma once

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxRhi/PhxRhi_Types.h>

namespace phx::resource::compiler
{
    struct IntermediateTexture
    {
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