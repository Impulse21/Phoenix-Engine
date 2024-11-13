#pragma once

#include <PhxEngine/Core/EnumClassFlags.h>

#include <DirectXTex.h>
#include <memory>
#include <string>
#include <stdint.h>

namespace PhxEngine
{
    class IFileSystem;
    class IBlob;
}
namespace PhxEngine::Pipeline
{
    enum class TexConversionFlags : uint8_t
    {
        SRGB = 1 << 0,          // Texture contains sRGB colors
        PreserveAlpha = 1 << 2, // Keep four channels
        NormalMap = 1 << 3,     // Texture contains normals
        BumpToNormal = 1 << 4,  // Generate a normal map from a bump map
        DefaultBC = 1 << 5,     // Apply standard block compression (BC1-5)
        QualityBC = 1 << 6,     // Apply quality block compression (BC6H/7)
        FlipVertical = 1 << 7,
    };

    enum class TextureType
    {
        Unknown = 0,
        DDS,
        TGA,
        HDR,
        EXR,
        WIC
    };

    PHX_ENUM_CLASS_FLAGS(TexConversionFlags)

    inline TextureType GetTextureType(std::string const& ext)
    {
        if (ext == ".dds")
        {
            return TextureType::DDS;
        }
        else if (ext == ".tga")
        {
            return TextureType::TGA;
        }
        else if (ext == ".hdr")
        {
            return TextureType::HDR;
        }
        else if (ext == ".exr")
        {
            return TextureType::EXR;
        }

        return TextureType::WIC;

    };

	class DDSTextureConverter
	{
	public:
        static std::unique_ptr<DirectX::ScratchImage> BuildDDS(IFileSystem* fileSystem, std::string const& filename, TexConversionFlags flags);
        static std::unique_ptr<DirectX::ScratchImage> BuildDDS(IBlob* blob, TextureType type, TexConversionFlags flags);
	};
}

