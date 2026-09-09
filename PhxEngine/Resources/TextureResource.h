#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/RelativePtr.h>
#include <PhxEngine/Core/RefCountPtr.h>

#include <PhxEngine/RHI/RHITypes.h>

#include "Resource.h"

namespace phx::resources
{
    struct TextureResource : public Resource
    {
        PHX_DECLARE_RESOURCE(TextureResource);

        struct CpuData
        {
            struct MipInfo
            {
                u32 byte_offset;
                u32 byte_size;
                u32 width;
                u32 height;
            };

            RelativePtr<MipInfo> mips;
            u32 mip_count;

            u32 width, height, depth, array_layers;
            u32 format;
        };

        MemoryBuffer cpu_data_buffer;
        TypedView<CpuData> cpu_data;

        rhi::TextureHandle   texture;
        rhi::DescriptorIndex bindless_index = rhi::kInvalidDescriptorIndex;

        void Dispose() override;
    };

    RefCountPtr<TextureResource> CreateTextureResource(MemoryBuffer&& file_bytes);
    RefCountPtr<TextureResource> LoadTextureResource(const char* virtual_path);
}
