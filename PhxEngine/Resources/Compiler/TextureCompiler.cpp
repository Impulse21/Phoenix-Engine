#include "TextureCompiler.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/BinaryBuilder.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Resources/Intermediate/IntermediateTexture.h>
#include <PhxEngine/Resources/TextureFileFormat.h>
#include <PhxEngine/Resources/TextureResource.h>
#include <PhxEngine/VFS/VFS.h>

#include <bc7enc.h>
#include <rgbcx.h>
#include <stb_image_resize2.h>

#include <algorithm>
#include <cstring>
#include <mutex>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "TextureCompiler" };

    void EnsureEncodersInitialized()
    {
        static std::once_flag flag;
        std::call_once(flag, []()
        {
            bc7enc_compress_block_init();
            rgbcx::init();
        });
    }

    rhi::Format PickFormat(TextureRole role)
    {
        switch (role)
        {
            case TextureRole::BaseColor:
            case TextureRole::Emissive:          return rhi::Format::BC7_UNORM_SRGB;
            case TextureRole::Normal:            return rhi::Format::BC5_UNORM;
            case TextureRole::MetallicRoughness: return rhi::Format::BC7_UNORM;
            case TextureRole::Occlusion:         return rhi::Format::BC4_UNORM;
            default:
                PHX_LOG_WARN(k_log, "Texture has an unclassified role -- defaulting to BC7_UNORM_SRGB");
                return rhi::Format::BC7_UNORM_SRGB;
        }
    }

    void Get4x4Block(const u8* pixels, u32 width, u32 height, u32 block_x, u32 block_y, u8 out[64])
    {
        for (u32 y = 0; y < 4; ++y)
        {
            const u32 src_y = std::min(block_y * 4 + y, height - 1);
            for (u32 x = 0; x < 4; ++x)
            {
                const u32 src_x = std::min(block_x * 4 + x, width - 1);
                std::memcpy(out + (y * 4 + x) * 4, pixels + (static_cast<size_t>(src_y) * width + src_x) * 4, 4);
            }
        }
    }

    struct MipSurface
    {
        std::vector<u8> pixels;
        u32 width  = 0;
        u32 height = 0;
    };

    std::vector<MipSurface> BuildMipChain(const u8* base_pixels, u32 width, u32 height, bool is_srgb)
    {
        std::vector<MipSurface> chain;
        chain.push_back(MipSurface{
            std::vector<u8>(base_pixels, base_pixels + static_cast<size_t>(width) * height * 4),
            width, height });

        u32 cur_w = width;
        u32 cur_h = height;
        while (cur_w > 1 || cur_h > 1)
        {
            const u32 next_w = std::max(1u, cur_w / 2);
            const u32 next_h = std::max(1u, cur_h / 2);

            MipSurface next;
            next.width  = next_w;
            next.height = next_h;
            next.pixels.resize(static_cast<size_t>(next_w) * next_h * 4);

            const MipSurface& prev = chain.back();
            if (is_srgb)
            {
                stbir_resize_uint8_srgb(prev.pixels.data(), static_cast<int>(cur_w), static_cast<int>(cur_h), 0,
                    next.pixels.data(), static_cast<int>(next_w), static_cast<int>(next_h), 0, STBIR_RGBA);
            }
            else
            {
                stbir_resize_uint8_linear(prev.pixels.data(), static_cast<int>(cur_w), static_cast<int>(cur_h), 0,
                    next.pixels.data(), static_cast<int>(next_w), static_cast<int>(next_h), 0, STBIR_RGBA);
            }

            chain.push_back(std::move(next));
            cur_w = next_w;
            cur_h = next_h;
        }

        return chain;
    }

    void EncodeSurfaceBC(const MipSurface& surface, rhi::Format format, u8* dest)
    {
        const u32 blocks_x = (surface.width + 3) / 4;
        const u32 blocks_y = (surface.height + 3) / 4;
        const u32 bytes_per_block = rhi::GetFormatBytesPerBlock(format);

        bc7enc_compress_block_params bc7_params{};
        if (format == rhi::Format::BC7_UNORM || format == rhi::Format::BC7_UNORM_SRGB)
        {
            bc7enc_compress_block_params_init(&bc7_params);
            if (!rhi::IsFormatSRGB(format))
                bc7enc_compress_block_params_init_linear_weights(&bc7_params);
            bc7_params.m_uber_level = BC7ENC_MAX_UBER_LEVEL;
        }

        for (u32 by = 0; by < blocks_y; ++by)
        {
            for (u32 bx = 0; bx < blocks_x; ++bx)
            {
                u8 block_pixels[64];
                Get4x4Block(surface.pixels.data(), surface.width, surface.height, bx, by, block_pixels);

                u8* block_dest = dest + (static_cast<size_t>(by) * blocks_x + bx) * bytes_per_block;
                switch (format)
                {
                    case rhi::Format::BC7_UNORM:
                    case rhi::Format::BC7_UNORM_SRGB:
                        bc7enc_compress_block(block_dest, block_pixels, &bc7_params);
                        break;
                    case rhi::Format::BC5_UNORM:
                        rgbcx::encode_bc5(block_dest, block_pixels, 0, 1, 4);
                        break;
                    case rhi::Format::BC4_UNORM:
                        rgbcx::encode_bc4(block_dest, block_pixels, 4);
                        break;
                    default:
                        PHX_ASSERT(false && "Unhandled BC format in TextureCompiler");
                        break;
                }
            }
        }
    }
}

CompiledTexture phx::resources::CompileTexture(const IntermediateTexture& texture, const TextureCompileOptions& options)
{
    EnsureEncodersInitialized();

    CompiledTexture compiled;
    compiled.width        = texture.width;
    compiled.height       = texture.height;
    compiled.depth        = texture.depth;
    compiled.array_layers = texture.array_layers;
    compiled.format       = PickFormat(texture.role);

    const bool is_srgb = rhi::IsFormatSRGB(compiled.format);
    const auto* base_pixels = reinterpret_cast<const u8*>(texture.pixel_data.Data());

    std::vector<MipSurface> mip_chain;
    if (options.generate_mips)
    {
        mip_chain = BuildMipChain(base_pixels, texture.width, texture.height, is_srgb);
    }
    else
    {
        MipSurface only;
        only.width  = texture.width;
        only.height = texture.height;
        only.pixels.assign(base_pixels, base_pixels + static_cast<size_t>(texture.width) * texture.height * 4);
        mip_chain.push_back(std::move(only));
    }

    BinaryBuilder<u32> mip_builder;
    std::vector<u32> mip_reserved_offsets(mip_chain.size());
    for (size_t i = 0; i < mip_chain.size(); ++i)
    {
        const u64 mip_size = rhi::GetSurfaceSize(compiled.format, mip_chain[i].width, mip_chain[i].height);
        mip_reserved_offsets[i] = mip_builder.Reserve(static_cast<size_t>(mip_size), 16u);
    }
    mip_builder.Commit();

    for (size_t i = 0; i < mip_chain.size(); ++i)
    {
        u8* dest = mip_builder.PlaceType<u8>(mip_reserved_offsets[i]);
        EncodeSurfaceBC(mip_chain[i], compiled.format, dest);
        compiled.mip_offsets.push_back(mip_reserved_offsets[i]);
    }

    compiled.packed_mips = mip_builder.Finalize();
    return compiled;
}

MemoryBuffer phx::resources::SerializeTexture(const CompiledTexture& texture, const std::string& source_path, const std::string& texture_name)
{
    std::string string_table;
    string_table.push_back('\0');

    auto add_string = [&string_table](const std::string& s) -> u32
    {
        if (s.empty())
            return 0;

        const u32 offset = static_cast<u32>(string_table.size());
        string_table.append(s);
        string_table.push_back('\0');
        return offset;
    };

    const u32 source_path_offset = add_string(source_path);

    u64 source_content_hash = 0;
    {
        MemoryBuffer source_bytes = VFS::ReadFile(source_path.c_str());
        if (!source_bytes.IsEmpty())
            source_content_hash = HashBytes(source_bytes.Data(), source_bytes.Size());
        else
            PHX_LOG_WARN(k_log, "Could not re-read source '{0}' (texture '{1}') to compute content hash", source_path, texture_name);
    }

    BinaryBuilder<u32> cpu_builder;
    const u32 cpu_data_offset = cpu_builder.Reserve<TextureResource::CpuData>();
    const u32 mip_info_offset = cpu_builder.ReserveArray<TextureResource::CpuData::MipInfo>(texture.mip_offsets.size());
    cpu_builder.Commit();

    auto* cpu_data = cpu_builder.PlaceType<TextureResource::CpuData>(cpu_data_offset);
    auto* mip_info = cpu_builder.PlaceType<TextureResource::CpuData::MipInfo>(mip_info_offset);

    cpu_data->mips.Set(mip_info);
    cpu_data->mip_count    = static_cast<u32>(texture.mip_offsets.size());
    cpu_data->width        = texture.width;
    cpu_data->height       = texture.height;
    cpu_data->depth        = texture.depth;
    cpu_data->array_layers = texture.array_layers;
    cpu_data->format       = static_cast<u32>(texture.format);

    for (size_t i = 0; i < texture.mip_offsets.size(); ++i)
    {
        const u32 mip_w = std::max(1u, texture.width >> i);
        const u32 mip_h = std::max(1u, texture.height >> i);
        mip_info[i].byte_offset = texture.mip_offsets[i];
        mip_info[i].byte_size   = static_cast<u32>(rhi::GetSurfaceSize(texture.format, mip_w, mip_h));
        mip_info[i].width       = mip_w;
        mip_info[i].height      = mip_h;
    }

    MemoryBuffer cpu_chunk_bytes = cpu_builder.Finalize();

    const u32 header_size           = static_cast<u32>(sizeof(ResourceFileHeader));
    const u32 chunk_table_size      = static_cast<u32>(sizeof(ChunkEntry) * 2);
    const u32 cpu_chunk_file_offset = AlignUp(header_size + chunk_table_size, 16u);
    const u32 gpu_chunk_file_offset = AlignUp(cpu_chunk_file_offset + static_cast<u32>(cpu_chunk_bytes.Size()), 16u);
    const u32 strings_file_offset   = AlignUp(gpu_chunk_file_offset + static_cast<u32>(texture.packed_mips.Size()), 16u);
    const u32 total_size            = strings_file_offset + static_cast<u32>(string_table.size());

    MemoryBuffer file_bytes(total_size, std::byte{ 0 });

    auto* header = reinterpret_cast<ResourceFileHeader*>(file_bytes.Data());
    header->magic              = kTextureFileMagic;
    header->version             = kTextureFileVersion;
    header->chunk_count         = 2;
    header->source_path_offset  = source_path_offset;
    header->source_content_hash = source_content_hash;
    header->strings_offset      = strings_file_offset;
    header->strings_size        = static_cast<u32>(string_table.size());

    auto* chunks = reinterpret_cast<ChunkEntry*>(file_bytes.Data() + header_size);
    chunks[0] = ChunkEntry{ kTextureChunkType_Cpu, cpu_chunk_file_offset, static_cast<u32>(cpu_chunk_bytes.Size()) };
    chunks[1] = ChunkEntry{ kTextureChunkType_Gpu, gpu_chunk_file_offset, static_cast<u32>(texture.packed_mips.Size()) };

    std::memcpy(file_bytes.Data() + cpu_chunk_file_offset, cpu_chunk_bytes.Data(), cpu_chunk_bytes.Size());
    std::memcpy(file_bytes.Data() + gpu_chunk_file_offset, texture.packed_mips.Data(), texture.packed_mips.Size());
    std::memcpy(file_bytes.Data() + strings_file_offset, string_table.data(), string_table.size());

    return file_bytes;
}

bool phx::resources::WriteTextureFile(const char* virtual_path, const MemoryBuffer& file_bytes)
{
    return VFS::WriteFile(virtual_path,
        Span<const u8>(reinterpret_cast<const u8*>(file_bytes.Data()), file_bytes.Size()));
}
