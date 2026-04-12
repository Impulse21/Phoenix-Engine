#pragma once

#include <PhxCore/Result.h>
#include <PhxCore/IO/IFileSystems.h>
#include <PhxRenderer/Compiler/IntermediateTexture.h>

#include <string>

namespace phx
{
    class IVirtualFileSystem;
}
namespace phx::renderer::compiler
{
    enum TexConversionFlags : uint8_t
    {
        kSRGB           = BIT(0),    // Texture contains sRGB colors
        kPreserveAlpha  = BIT(1),    // Keep four channels
        kNormalMap      = BIT(2),    // Texture contains normals
        kBumpToNormal   = BIT(3),    // Generate a normal map from a bump map
        kDefaultBC      = BIT(4),    // Apply standard block compression (BC1-5)
        kQualityBC      = BIT(5),    // Apply quality block compression (BC6H/7)
        kFlipVertical   = BIT(6),
    };

    struct TextureCompileDescriptor
    {
        std::string virtual_input_path;
        std::string virtual_output_path;
        TexConversionFlags flags;
        uint32_t quality_value = 4;
    };

    inline TexConversionFlags TextureOptions(bool sRGB, bool has_alpha = false, bool invert_y = false)
    {
        // 2. Accumulate inside a raw integer type first
        uint8_t flags = 0;

        if (sRGB)       flags |= kSRGB;
        if (has_alpha)  flags |= kPreserveAlpha;
        if (invert_y)   flags |= kFlipVertical;

        // 3. Explicitly cast back to the Enum
        return static_cast<TexConversionFlags>(flags);
    }

	namespace TextureCompiler
	{
        Result<compiler::IntermediateTexture> Compile(phx::IRootFileSystem* vfs, TextureCompileDescriptor const& desc);
	}
}
