#pragma once

#include <stdint.h>
namespace phx
{
    enum TexConversionFlags
    {
        kSRGB = 1 << 0,          // Texture contains sRGB colors
        kPreserveAlpha = 1 << 2, // Keep four channels
        kNormalMap = 1 << 3,     // Texture contains normals
        kBumpToNormal = 1 << 4,  // Generate a normal map from a bump map
        kDefaultBC = 1 << 5,    // Apply standard block compression (BC1-5)
        kQualityBC = 1 << 6,    // Apply quality block compression (BC6H/7)
        kFlipVertical = 1 << 7,
    };

    inline uint8_t TextureOptions(bool sRGB, bool hasAlpha = false, bool invertY = false)
    {
        return (sRGB ? kSRGB : 0) | (hasAlpha ? kPreserveAlpha : 0) | (invertY ? kFlipVertical : 0);
    }
}