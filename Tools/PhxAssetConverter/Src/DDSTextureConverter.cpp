#include "DDSTextureConverter.h"
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Log.h>

using namespace PhxEngine;

std::unique_ptr<DirectX::ScratchImage> PhxEngine::Pipeline::DDSTextureConverter::BuildDDS(Core::IFileSystem* fileSystem, std::string const& filename, TexConversionFlags flags)
{
    std::unique_ptr<Core::IBlob> blob = fileSystem->ReadFile(filename);
    PHX_LOG_INFO("Converting file '%s' to DDS.", filename.c_str());
    return DDSTextureConverter::BuildDDS(blob.get(), GetTextureType(Core::FileSystem::GetFileExt(filename)), flags);
}


// https://github.com/microsoft/DirectStorage/blob/main/Samples/BulkLoadDemo/README.md
std::unique_ptr<DirectX::ScratchImage> PhxEngine::Pipeline::DDSTextureConverter::BuildDDS(Core::IBlob* blob, TextureType type, TexConversionFlags flags)
{
    bool bInterpretAsSRGB = EnumHasAnyFlags(flags, TexConversionFlags::SRGB);
    bool bPreserveAlpha = EnumHasAnyFlags(flags, TexConversionFlags::PreserveAlpha);
    bool bContainsNormals = EnumHasAnyFlags(flags, TexConversionFlags::NormalMap);
    bool bBumpMap = EnumHasAnyFlags(flags, TexConversionFlags::BumpToNormal);
    bool bBlockCompress = EnumHasAnyFlags(flags, TexConversionFlags::DefaultBC);
    bool bUseBestBC = EnumHasAnyFlags(flags, TexConversionFlags::QualityBC);
    bool bFlipImage = EnumHasAnyFlags(flags, TexConversionFlags::FlipVertical);

    // Can't be both
    assert(!bInterpretAsSRGB || !bContainsNormals);
    assert(!bPreserveAlpha || !bContainsNormals);

    // Load texture image
    DirectX::TexMetadata info;
    std::unique_ptr<DirectX::ScratchImage> image = std::make_unique<DirectX::ScratchImage>();

    switch (type)
    {
    case TextureType::DDS:
        HRESULT hr = DirectX::LoadFromDDSMemory(blob->Data(), blob->Size(), DirectX::DDS_FLAGS_NONE, &info, *image);
        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not load texture DDS texture from memory (DDS: %08X).", hr);
            return {};
        }
        break;
    case TextureType::TGA:
        HRESULT hr = DirectX::LoadFromTGAMemory(blob->Data(), blob->Size(), &info, *image);
        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not load texture TGA texture from memory (DDS: %08X).", hr);
            return {};
        }
        break;
    case TextureType::HDR:
        HRESULT hr = DirectX::LoadFromHDRMemory(blob->Data(), blob->Size(), &info, *image);
        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not load texture HDR texture from memory (DDS: %08X).", hr);
            return {};
        }
        break;
    case TextureType::EXR:
        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not load texture EXR texture from memory (DDS: %08X).", hr);
            return {};
        }
        break;
    case TextureType::WIC:
        DirectX::WIC_FLAGS wicFlags = DirectX::WIC_FLAGS_NONE;
        HRESULT hr = DirectX::LoadFromWICMemory(blob->Data(), blob->Size(), wicFlags, &info, *image);
        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not load texture HDR texture from memory (DDS: %08X).", hr);
            return {};
        }
        break;
    case TextureType::Unknown:
    default:
        assert(0 && "UNKNOWN TEXTURE TYPE");
        return {};
    }

    if (info.width > 16384 || info.height > 16384)
    {
        PHX_LOG_ERROR("Texture size (%Iu,%Iu) too large for feature level 11.0 or later (16384)", info.width, info.height);
        return {};
    }

    if (bFlipImage)
    {
        std::unique_ptr<DirectX::ScratchImage> timage = std::make_unique<DirectX::ScratchImage>();

        HRESULT hr = DirectX::FlipRotate(image->GetImages()[0], DirectX::TEX_FR_FLIP_VERTICAL, *timage);

        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not flip image (%08X).\n", hr);
        }
        else
        {
            image.swap(timage);
        }
    }

    DXGI_FORMAT tformat;
    DXGI_FORMAT cformat;
    if (type == TextureType::HDR)
    {
        tformat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        cformat = bBlockCompress ? DXGI_FORMAT_BC6H_UF16 : DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    }
    else if (bBlockCompress)
    {
        tformat = bInterpretAsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        if (bUseBestBC)
            cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
        else if (bPreserveAlpha)
            cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
        else
            cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
    }
    else
    {
        cformat = tformat = bInterpretAsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    if (bBumpMap)
    {
        std::unique_ptr<DirectX::ScratchImage> timage = std::make_unique<DirectX::ScratchImage>();

        HRESULT hr = DirectX::ComputeNormalMap(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DirectX::CNMAP_CHANNEL_LUMINANCE, 10.0f, tformat, *timage);

        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not compute normal map (%08X).", hr);
        }
        else
        {
            image.swap(timage);
            info.format = tformat;
        }
    }
    else if (info.format != tformat)
    {
        std::unique_ptr<DirectX::ScratchImage> timage = std::make_unique<DirectX::ScratchImage>();

        HRESULT hr = DirectX::Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
            tformat, DirectX::TEX_FILTER_DEFAULT, 0.5f, *timage);

        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Could not convert (%08X).\n", hr);
        }
        else
        {
            image.swap(timage);
            info.format = tformat;
        }
    }

    // Handle mipmaps
    if (info.mipLevels == 1)
    {
        std::unique_ptr<DirectX::ScratchImage> timage = std::make_unique<DirectX::ScratchImage>();

        HRESULT hr = DirectX::GenerateMipMaps(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, *timage);

        if (FAILED(hr))
        {
            PHX_LOG_ERROR("Failing generating mimaps for (WIC: %08X).", hr);
        }
        else
        {
            image.swap(timage);
        }
    }

    // Handle compression
    if (bBlockCompress)
    {
        if (info.width % 4 || info.height % 4)
        {
            PHX_LOG_ERROR("Texture size (%Iux%Iu) not a multiple of 4, so skipping compress\n", info.width, info.height);
        }
        else
        {
            std::unique_ptr<DirectX::ScratchImage> timage = std::make_unique<DirectX::ScratchImage>();

            HRESULT hr = Compress(image->GetImages(), image->GetImageCount(), image->GetMetadata(), cformat, DirectX::TEX_COMPRESS_DEFAULT, 0.5f, *timage);
            if (FAILED(hr))
            {
                PHX_LOG_ERROR("Failing compressing (WIC: %08X).", hr);
            }
            else
            {
                image.swap(timage);
            }
        }
    }

    return image;
}
