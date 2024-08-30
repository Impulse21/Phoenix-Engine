#include "phxTextureConvert.h"

#include <phxBaseInclude.h>

#include <Core/phxVirtualFileSystem.h>

#include <memory>
#include <DirectXTex.h>

using namespace phx;
using namespace phx::TextureCompiler;
using namespace DirectX;

namespace
{
    std::unique_ptr<ScratchImage> BuildDDS(IFileSystem& fs, std::string const& filename, uint32_t flags)
	{

	}

    void SaveToDDSFile()
    {

    }

    bool ConvertToDDS(IFileSystem& fs, std::string const& filename, uint32_t flags)
    {
        std::unique_ptr<ScratchImage> image = BuildDDS(fs, filename, flags);

        if (!image)
            return false;

        // Rename file extension to DDS
        const std::string dest = FileSystem::GetFileNameWithoutExt(filename) + ".dds";

        // TODO: WSTRING
        std::wstring wDest;
        phx::StringConvert(dest, wDest);

        // Save DDS
        HRESULT hr = SaveToDDSFile(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DDS_FLAGS_NONE, wDest.c_str());
        if (FAILED(hr))
        {
            PHX_ERROR("Could not write texture to file \"%ws\" (%08X).\n", wDest.c_str(), hr);
            return false;
        }

        return true;
    }
}
void phx::TextureCompiler::CompileOnDemand(IFileSystem& fs, std::string const& filename, uint32_t flags)
{
    std::string ddsFile = FileSystem::GetFileNameWithoutExt(filename) + ".dds";

    const bool srcFileMissing = fs.FileExists(filename);
    const bool ddsFileMissing = fs.FileExists(ddsFile);

    if (srcFileMissing && ddsFileMissing)
    {
        PHX_ERROR("Texture %s is missing.", FileSystem::GetFileNameWithoutExt(filename));
        return;
    }

    // If we can find the source texture and the DDS file is older, reconvert.
    if (ddsFileMissing || !srcFileMissing && FileSystem::GetLastWriteTime(ddsFile) < FileSystem::GetLastWriteTime(filename))
    {
        PHX_INFO("DDS texture %s missing or older than source.Rebuilding", FileSystem::GetFileNameWithoutExt(filename));
        ConvertToDDS(fs, filename, flags);
    }
}
