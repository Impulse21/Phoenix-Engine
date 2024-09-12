#include "pch.h"

#include "phxTextureConvert.h"

#include <phxBaseInclude.h>

#include <Core/phxVirtualFileSystem.h>

#include <memory>

using namespace phx;
using namespace phx::TextureCompiler;
using namespace DirectX;

#define GetFlag(f) ((flags & f) != 0)
namespace
{

    bool ConvertToDDS(std::string const& filename, uint32_t flags)
    {
        std::unique_ptr<ScratchImage> image = phx::TextureCompiler::BuildDDS(filename, flags);

        if (!image)
            return false;

        // Rename file extension to DDS
        const std::string dest = FileSystem::GetFileNameWithoutExt(filename) + ".dds";

        std::wstring wDest;
        phx::StringConvert(dest, wDest);

        // Save DDS
        HRESULT hr = SaveToDDSFile(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DDS_FLAGS_NONE, wDest.c_str());
        if (FAILED(hr))
        {
            PHX_ERROR("Could not write texture to file \"%s\" ().\n", dest.c_str());
            return false;
        }

        return true;
    }
}


std::unique_ptr<ScratchImage> phx::TextureCompiler::BuildDDS(std::string const& filename, uint32_t flags)
{
    bool bInterpretAsSRGB = GetFlag(kSRGB);
    bool bPreserveAlpha = GetFlag(kPreserveAlpha);
    bool bContainsNormals = GetFlag(kNormalMap);
    bool bBumpMap = GetFlag(kBumpToNormal);
    bool bBlockCompress = GetFlag(kDefaultBC);
    bool bUseBestBC = GetFlag(kQualityBC);
    bool bFlipImage = GetFlag(kFlipVertical);

    // Can't be both
    assert(!bInterpretAsSRGB || !bContainsNormals);
    assert(!bPreserveAlpha || !bContainsNormals);

    PHX_INFO("Converting file \"%s\" to DDS.", filename.c_str());

    // Get extension as utf8 (ascii)
    std::string ext = FileSystem::GetFileExt(filename);

    // Load texture image
    TexMetadata info;
    std::unique_ptr<ScratchImage> image = std::make_unique<ScratchImage>();

    std::wstring wFilename;
    StringConvert(filename, wFilename);
    bool isDDS = false;
    bool isHDR = false;
    if (ext == ".dds")
    {
        isDDS = true;
        // TODO:  It might be desired to compress or recompress existing DDS files
        //Utility::Printf("Ignoring existing DDS \"%s\".\n", filePath.c_str());
        //return false;

        HRESULT hr = LoadFromDDSFile(wFilename.c_str(), DDS_FLAGS_NONE, &info, *image);
        if (FAILED(hr))
        {
            PHX_ERROR("Could not load texture \"%s\" (DDS).", filename.c_str());
            return nullptr;
        }
    }
    else if (ext == ".tga")
    {
        HRESULT hr = LoadFromTGAFile(wFilename.c_str(), &info, *image);
        if (FAILED(hr))
        {
            PHX_ERROR("Could not load texture \"%s\" (TGA).", filename.c_str());
            return nullptr;
        }
    }
    else if (ext == "hdr")
    {
        isHDR = true;
        HRESULT hr = LoadFromHDRFile(wFilename.c_str(), &info, *image);
        if (FAILED(hr))
        {
            PHX_ERROR("Could not load texture \"%s\" (HDR).", filename.c_str());
            return nullptr;
        }
    }
    else if (ext == "exr")
    {
#ifdef USE_OPENEXR
        isHDR = true;
        HRESULT hr = LoadFromEXRFile(filePath.c_str(), &info, *image);
        if (FAILED(hr))
        {
            PHX_ERROR("Could not load texture \"%s\" (EXR).", filename.c_str());
            return nullptr;
        }
#else
        PHX_ERROR("OpenEXR not supported for this build of the content exporter");
        return nullptr;
#endif
    }
    else
    {
        WIC_FLAGS wicFlags = WIC_FLAGS_NONE;
        HRESULT hr = LoadFromWICFile(wFilename.c_str(), wicFlags, &info, *image);
        if (FAILED(hr))
        {
            PHX_ERROR("Could not load texture \"%s\" (WIC).", filename.c_str());
            return nullptr;
        }
    }

    if (info.width > 16384 || info.height > 16384)
    {
        PHX_ERROR("Texture size (%Iu,%Iu) too large for feature level 11.0 or later (16384) \"%s\"", info.width, info.height, filename.c_str());
        return nullptr;
    }

    if (bFlipImage)
    {
        std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();

        HRESULT hr = FlipRotate(image->GetImages()[0], TEX_FR_FLIP_VERTICAL, *timage);

        if (FAILED(hr))
        {
            PHX_ERROR("Could not flip image \"%s\" ().", filename.c_str());
        }
        else
        {
            image.swap(timage);
        }
    }

    DXGI_FORMAT tformat;
    DXGI_FORMAT cformat;

    if (isHDR)
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
        std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();

        HRESULT hr = ComputeNormalMap(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
            CNMAP_CHANNEL_LUMINANCE, 10.0f, tformat, *timage);

        if (FAILED(hr))
        {
            PHX_ERROR("Could not compute normal map for \"%s\" ().", filename.c_str());
        }
        else
        {
            image.swap(timage);
            info.format = tformat;
        }
    }
    else if (info.format != tformat)
    {
        std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();

        HRESULT hr = Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
            tformat, TEX_FILTER_DEFAULT, 0.5f, *timage);

        if (FAILED(hr))
        {
            PHX_ERROR("Could not convert \"%s\" ().", filename.c_str());
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
        std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();
        HRESULT hr = GenerateMipMaps(image->GetImages(), image->GetImageCount(), image->GetMetadata(), TEX_FILTER_DEFAULT, 0, *timage);

        if (FAILED(hr))
        {
            PHX_ERROR("Failing generating mimaps for \"%s\" (WIC:)", filename.c_str());
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
            PHX_ERROR("Texture size (%Iux%Iu) not a multiple of 4 \"%s\", so skipping compress", info.width, info.height, filename.c_str());
        }
        else
        {
            std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();

            HRESULT hr = Compress(image->GetImages(), image->GetImageCount(), image->GetMetadata(), cformat, TEX_COMPRESS_DEFAULT, 0.5f, *timage);
            if (FAILED(hr))
            {
                PHX_ERROR("Failing compressing \"%s\" (WIC:).", filename.c_str());
            }
            else
            {
                image.swap(timage);
            }
        }
    }

    return image;
}

void phx::TextureCompiler::CompileOnDemand(IFileSystem& fs, std::string const& filename, uint32_t flags)
{
    std::string ddsFile = FileSystem::GetFileNameWithoutExt(filename) + ".dds";

    const bool srcFileMissing = fs.FileExists(filename);
    const bool ddsFileMissing = fs.FileExists(ddsFile);

    if (srcFileMissing && ddsFileMissing)
    {
        PHX_ERROR("Texture %s is missing.", FileSystem::GetFileNameWithoutExt(filename).c_str());
        return;
    }

    // If we can find the source texture and the DDS file is older, reconvert.
    if (ddsFileMissing || !srcFileMissing && FileSystem::GetLastWriteTime(ddsFile) < FileSystem::GetLastWriteTime(filename))
    {
        PHX_INFO("DDS texture %s missing or older than source.Rebuilding", FileSystem::GetFileNameWithoutExt(filename).c_str());
        ConvertToDDS(filename, flags);
    }
}
