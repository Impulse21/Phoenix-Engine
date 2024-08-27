#pragma once

#include <stdint.h>
namespace phx
{
    enum TexConversionFlags
    {
        kSRGB = 1,          // Texture contains sRGB colors
        kPreserveAlpha = 2, // Keep four channels
        kNormalMap = 4,     // Texture contains normals
        kBumpToNormal = 8,  // Generate a normal map from a bump map
        kDefaultBC = 16,    // Apply standard block compression (BC1-5)
        kQualityBC = 32,    // Apply quality block compression (BC6H/7)
        kFlipVertical = 64,
    };

    inline uint8_t TextureOptions(bool sRGB, bool hasAlpha = false, bool invertY = false)
    {
        return (sRGB ? kSRGB : 0) | (hasAlpha ? kPreserveAlpha : 0) | (invertY ? kFlipVertical : 0);
    }
}