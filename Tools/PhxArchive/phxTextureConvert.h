#pragma once

#include <stdint.h>
#include <phxBaseInclude.h>

namespace phx
{
    class IFileSystem;

    enum TexConversionFlags
    {
        kSRGB           = BIT(0),   // Texture contains sRGB colors
        kPreserveAlpha  = BIT(1),   // Keep four channels
        kNormalMap      = BIT(2),   // Texture contains normals
        kBumpToNormal   = BIT(3),   // Generate a normal map from a bump map
        kDefaultBC      = BIT(4),   // Apply standard block compression (BC1-5)
        kQualityBC      = BIT(5),   // Apply quality block compression (BC6H/7)
        kFlipVertical   = BIT(6),
    };

    inline uint8_t TextureOptions(bool sRGB, bool hasAlpha = false, bool invertY = false)
    {
        return (sRGB ? kSRGB : 0) | (hasAlpha ? kPreserveAlpha : 0) | (invertY ? kFlipVertical : 0);
    }

    namespace TextureCompiler
    {
        void CompileOnDemand(IFileSystem& fs, std::string const& filename, uint32_t flags);
    }
}