#pragma once

#include "PhxRhi_Types.h"

namespace phx::rhi 
{
    struct FormatInfo
    {
        uint32_t bytes_per_block;
        uint32_t block_width;
        uint32_t block_height;
        bool is_compressed;
    };

    inline uint64_t GetSurfaceSize(Format format, uint32_t width, uint32_t height, uint32_t depth);

    constexpr FormatInfo GetFormatInfo(Format format)
    {
        switch (format)
        {
        // --------------------------------------------------------
        // 8-BIT / 1 BYTE
        // --------------------------------------------------------
        case Format::R8_UINT:
        case Format::R8_SINT:
        case Format::R8_UNORM:
        case Format::R8_SNORM:
        {
            return { 1, 1, 1, false };
        }

        // --------------------------------------------------------
        // 16-BIT / 2 BYTES
        // --------------------------------------------------------
        case Format::RG8_UINT:
        case Format::RG8_SINT:
        case Format::RG8_UNORM:
        case Format::RG8_SNORM:
        case Format::R16_UINT:
        case Format::R16_SINT:
        case Format::R16_UNORM:
        case Format::R16_SNORM:
        case Format::R16_FLOAT:
        case Format::BGRA4_UNORM:     // 4*4 bits = 16 bits
        case Format::B5G6R5_UNORM:    // 16 bits
        case Format::B5G5R5A1_UNORM:  // 16 bits
        case Format::D16:             // Depth 16
        {
            return { 2, 1, 1, false };
        }

        // --------------------------------------------------------
        // 32-BIT / 4 BYTES
        // --------------------------------------------------------
        case Format::RGBA8_UINT:
        case Format::RGBA8_SINT:
        case Format::RGBA8_UNORM:
        case Format::RGBA8_SNORM:
        case Format::BGRA8_UNORM:
        case Format::SRGBA8_UNORM:
        case Format::SBGRA8_UNORM:
        case Format::R10G10B10A2_UNORM:
        case Format::R11G11B10_FLOAT:
        case Format::RG16_UINT:
        case Format::RG16_SINT:
        case Format::RG16_UNORM:
        case Format::RG16_SNORM:
        case Format::RG16_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        case Format::R32_FLOAT:
        case Format::D32:
        case Format::D24S8:       // Usually packed into 32 bits (24+8)
        case Format::X24G8_UINT:
        {
            return { 4, 1, 1, false };
        }

        // --------------------------------------------------------
        // 64-BIT / 8 BYTES
        // --------------------------------------------------------
        case Format::RGBA16_UINT:
        case Format::RGBA16_SINT:
        case Format::RGBA16_FLOAT:
        case Format::RGBA16_UNORM:
        case Format::RGBA16_SNORM:
        case Format::RG32_UINT:
        case Format::RG32_SINT:
        case Format::RG32_FLOAT:
        case Format::D32S8:       // 32 Depth + 8 Stencil (aligned to 64)
        case Format::X32G8_UINT:
        {
            return { 8, 1, 1, false };
        }

        // --------------------------------------------------------
        // 96-BIT / 12 BYTES
        // --------------------------------------------------------
        case Format::RGB32_UINT:
        case Format::RGB32_SINT:
        case Format::RGB32_FLOAT:
        {
            return { 12, 1, 1, false };
        }

        // --------------------------------------------------------
        // 128-BIT / 16 BYTES
        // --------------------------------------------------------
        case Format::RGBA32_UINT:
        case Format::RGBA32_SINT:
        case Format::RGBA32_FLOAT:
        {
            return { 16, 1, 1, false };
        }

        // --------------------------------------------------------
        // BLOCK COMPRESSED - 8 BYTES (64-bit block)
        // --------------------------------------------------------
        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB:
        case Format::BC4_UNORM:
        case Format::BC4_SNORM:
        {
            return { 8, 4, 4, true };
        }

        // --------------------------------------------------------
        // BLOCK COMPRESSED - 16 BYTES (128-bit block)
        // --------------------------------------------------------
        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB:
        case Format::BC5_UNORM:
        case Format::BC5_SNORM:
        case Format::BC6H_UFLOAT:
        case Format::BC6H_SFLOAT:
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB:
        {
            return { 16, 4, 4, true };
        }

        case Format::UNKNOWN:
        case Format::COUNT:
        default:
        {
            return { 0, 0, 0, false };
        }
        }
    }

    inline uint64_t GetSurfaceSize(Format format, uint32_t width, uint32_t height, uint32_t depth)
    {
        FormatInfo info = GetFormatInfo(format);

        if (info.is_compressed)
        {
            uint32_t num_blocks_x = (width + info.block_width - 1) / info.block_width;
            uint32_t num_blocks_y = (height + info.block_height - 1) / info.block_height;

            return static_cast<uint64_t>(num_blocks_x) * num_blocks_y * depth * info.bytes_per_block;
        }
        else
        {
            return static_cast<uint64_t>(width) * height * depth * info.bytes_per_block;
        }
    }
}
