#include "PhxRenderer_pch.h"
#include <PhxRenderer/Compiler/TextureCompiler.h>

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/IO/FileUtils.h>

#include <PhxRhi/PhxRhi_Utils.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <bc7enc.h>
#include <rgbcx.h>

using namespace  phx;
using namespace  phx::renderer;
using namespace  phx::resource::compiler;

namespace
{
    // four channel surface.
    struct Surface
    {
        int width;
        int height;
        std::vector<uint8_t> pixels;
    };

    void Get4x4Block(const Surface& src, int bx, int by, std::byte* out_rgba_64)
    {
        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                // Clamp coordinates to image dimensions (Edge Extension)
                int px = std::min(bx * 4 + x, src.width - 1);
                int py = std::min(by * 4 + y, src.height - 1);

                int src_idx = (py * src.width + px) * 4;
                int dst_idx = (y * 4 + x) * 4;

                memcpy(&out_rgba_64[dst_idx], &src.pixels[src_idx], 4);
            }
        }
    }
}

phx::Result<IntermediateTexture> TextureCompiler::Compile(phx::IVirtualFileSystem* vfs, TextureCompileDescriptor const& desc)
{
    static std::once_flag s_initFlag;
    std::call_once(
        s_initFlag, []() {
            bc7enc_compress_block_init();
    });

    std::string input_path = vfs->ResolveVirtualToPhysicalPath(desc.virtual_input_path).ValueOr("");
    std::string output_path = vfs->ResolveVirtualToPhysicalPath(desc.virtual_output_path).ValueOr("");

    int w, h, channels;

    uint8_t* raw_data = stbi_load(input_path.c_str(), &w, &h, &channels, 4);
    if (!raw_data)
    {
        PHX_ERROR("Failed to load image: '{0}'", desc.virtual_input_path);
        return Unexpected(ResultError::Failure);
    }

    std::vector<Surface> mip_chain;
    Surface& level0 = mip_chain.emplace_back();
    level0.width = w;
    level0.height = h;
    level0.pixels.assign(raw_data, raw_data + (w * h * 4));

    stbi_image_free(raw_data);

    int current_width = w;
    int current_height = h;

    //bool is_normal_map = desc.flags & TexConversionFlags::kNormalMap;
    const bool is_srgb = desc.flags & TexConversionFlags::kSRGB;
    const rhi::Format target_format = is_srgb ? rhi::Format::BC7_UNORM_SRGB : rhi::Format::BC7_UNORM;

    while (current_width > 1 || current_height> 1)
    {
        int next_width = std::max(1, current_width / 2);
        int next_height = std::max(1, current_height / 2);

        size_t prev_index = mip_chain.size() - 1;

        Surface& nextLevel = mip_chain.emplace_back();
        nextLevel.width = next_width;
        nextLevel.height = next_height;
        nextLevel.pixels.resize(next_width * next_height * 4);

        Surface& prevLevel = mip_chain[prev_index];

        if (is_srgb)
        {
            stbir_resize_uint8_srgb(
                prevLevel.pixels.data(), prevLevel.width, prevLevel.height, 0,
                nextLevel.pixels.data(), nextLevel.width, nextLevel.height, 0,
                STBIR_RGBA);
        }
        else
        {
            stbir_resize_uint8_linear(
                prevLevel.pixels.data(), prevLevel.width, prevLevel.height, 0,
                nextLevel.pixels.data(), nextLevel.width, nextLevel.height, 0,
                STBIR_RGBA);
        }

        current_width = next_width;
        current_height = next_height;;
    }

    IntermediateTexture result = 
    {
        .width = static_cast<uint32_t>(w),
        .height = static_cast<uint32_t>(h),
        .format = target_format,
	};

    result.mip_offsets.reserve(mip_chain.size());

    size_t total_data_size = 0;
    for (const auto& surface : mip_chain)
    {
        result.mip_offsets.push_back(total_data_size);
        const uint64_t mip_size = rhi::GetSurfaceSize(target_format, surface.width, surface.height, 1);
        total_data_size += mip_size;
    }

    // Allocate the blob ONCE using unique_ptr
    result.pixel_data = phx::MemoryBuffer(total_data_size);

    bc7enc_compress_block_params params;
    bc7enc_compress_block_params_init(&params);

    if (is_srgb)
    {
        bc7enc_compress_block_params_init_perceptual_weights(&params);
    }
    else
    {
        bc7enc_compress_block_params_init_linear_weights(&params);
    }

    params.m_uber_level = desc.quality_value;

    std::byte* dest_ptr = result.pixel_data.Data();
    for (const auto& surface : mip_chain)
    {
        int blocks_x = (surface.width + 3) / 4;
        int blocks_y = (surface.height + 3) / 4;

        // Loop over 4x4 blocks
        for (int by = 0; by < blocks_y; ++by)
        {
            for (int bx = 0; bx < blocks_x; ++bx)
            {
                std::byte block_pixels[64];
                Get4x4Block(surface, bx, by, block_pixels);
                bc7enc_compress_block(dest_ptr, block_pixels, &params);

                dest_ptr += 16;
            }
        }
    }

    return result;
}
