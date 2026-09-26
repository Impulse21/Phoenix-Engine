#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/RHI/RHITypes.h>

#include <string>
#include <vector>

namespace phx::resources
{
    struct IntermediateTexture;

    struct TextureCompileOptions
    {
        u32  quality       = 4;
        bool generate_mips = true;
    };

    struct CompiledTexture
    {
        MemoryBuffer packed_mips;
        std::vector<u32> mip_offsets;

        u32 width = 0, height = 0, depth = 1, array_layers = 1;
        rhi::Format format = rhi::Format::UNKNOWN;
    };

    CompiledTexture CompileTexture(const IntermediateTexture& texture, const TextureCompileOptions& options = {});
    MemoryBuffer SerializeTexture(const CompiledTexture& texture, const std::string& source_path, const std::string& texture_name);
    bool WriteTextureFile(const char* virtual_path, const MemoryBuffer& file_bytes);
}
