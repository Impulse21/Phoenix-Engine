#include "phxpch.h"

#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include "PhxEngine/Core/Helpers.h"

#include "DirectXTex.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Scene;

using namespace DirectX;


namespace PhxEngine::Graphics
{
    struct LoadedTexture
    {
        std::string Path;
        bool ForceSRGB;
        RHI::TextureHandle RhiTexture;

        // std::shared_ptr<Core::IBlob> Data;
        std::vector<RHI::SubresourceData> SubresourceData;

        DirectX::TexMetadata Metadata;
        DirectX::ScratchImage ScratchImage;
    };
}

struct FormatMapping
{
    RHI::RHIFormat PhxRHIFormat;
    DXGI_FORMAT DxgiFormat;
    uint32_t BitsPerPixel;
};

const FormatMapping g_FormatMappings[] = {
    { RHI::RHIFormat::UNKNOWN,              DXGI_FORMAT_UNKNOWN,                0 },
    { RHI::RHIFormat::R8_UINT,              DXGI_FORMAT_R8_UINT,                8 },
    { RHI::RHIFormat::R8_SINT,              DXGI_FORMAT_R8_SINT,                8 },
    { RHI::RHIFormat::R8_UNORM,             DXGI_FORMAT_R8_UNORM,               8 },
    { RHI::RHIFormat::R8_SNORM,             DXGI_FORMAT_R8_SNORM,               8 },
    { RHI::RHIFormat::RG8_UINT,             DXGI_FORMAT_R8G8_UINT,              16 },
    { RHI::RHIFormat::RG8_SINT,             DXGI_FORMAT_R8G8_SINT,              16 },
    { RHI::RHIFormat::RG8_UNORM,            DXGI_FORMAT_R8G8_UNORM,             16 },
    { RHI::RHIFormat::RG8_SNORM,            DXGI_FORMAT_R8G8_SNORM,             16 },
    { RHI::RHIFormat::R16_UINT,             DXGI_FORMAT_R16_UINT,               16 },
    { RHI::RHIFormat::R16_SINT,             DXGI_FORMAT_R16_SINT,               16 },
    { RHI::RHIFormat::R16_UNORM,            DXGI_FORMAT_R16_UNORM,              16 },
    { RHI::RHIFormat::R16_SNORM,            DXGI_FORMAT_R16_SNORM,              16 },
    { RHI::RHIFormat::R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,              16 },
    { RHI::RHIFormat::BGRA4_UNORM,          DXGI_FORMAT_B4G4R4A4_UNORM,         16 },
    { RHI::RHIFormat::B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           16 },
    { RHI::RHIFormat::B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         16 },
    { RHI::RHIFormat::RGBA8_UINT,           DXGI_FORMAT_R8G8B8A8_UINT,          32 },
    { RHI::RHIFormat::RGBA8_SINT,           DXGI_FORMAT_R8G8B8A8_SINT,          32 },
    { RHI::RHIFormat::RGBA8_UNORM,          DXGI_FORMAT_R8G8B8A8_UNORM,         32 },
    { RHI::RHIFormat::RGBA8_SNORM,          DXGI_FORMAT_R8G8B8A8_SNORM,         32 },
    { RHI::RHIFormat::BGRA8_UNORM,          DXGI_FORMAT_B8G8R8A8_UNORM,         32 },
    { RHI::RHIFormat::SRGBA8_UNORM,         DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    32 },
    { RHI::RHIFormat::SBGRA8_UNORM,         DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,    32 },
    { RHI::RHIFormat::R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,      32 },
    { RHI::RHIFormat::R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        32 },
    { RHI::RHIFormat::RG16_UINT,            DXGI_FORMAT_R16G16_UINT,            32 },
    { RHI::RHIFormat::RG16_SINT,            DXGI_FORMAT_R16G16_SINT,            32 },
    { RHI::RHIFormat::RG16_UNORM,           DXGI_FORMAT_R16G16_UNORM,           32 },
    { RHI::RHIFormat::RG16_SNORM,           DXGI_FORMAT_R16G16_SNORM,           32 },
    { RHI::RHIFormat::RG16_FLOAT,           DXGI_FORMAT_R16G16_FLOAT,           32 },
    { RHI::RHIFormat::R32_UINT,             DXGI_FORMAT_R32_UINT,               32 },
    { RHI::RHIFormat::R32_SINT,             DXGI_FORMAT_R32_SINT,               32 },
    { RHI::RHIFormat::R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,              32 },
    { RHI::RHIFormat::RGBA16_UINT,          DXGI_FORMAT_R16G16B16A16_UINT,      64 },
    { RHI::RHIFormat::RGBA16_SINT,          DXGI_FORMAT_R16G16B16A16_SINT,      64 },
    { RHI::RHIFormat::RGBA16_FLOAT,         DXGI_FORMAT_R16G16B16A16_FLOAT,     64 },
    { RHI::RHIFormat::RGBA16_UNORM,         DXGI_FORMAT_R16G16B16A16_UNORM,     64 },
    { RHI::RHIFormat::RGBA16_SNORM,         DXGI_FORMAT_R16G16B16A16_SNORM,     64 },
    { RHI::RHIFormat::RG32_UINT,            DXGI_FORMAT_R32G32_UINT,            64 },
    { RHI::RHIFormat::RG32_SINT,            DXGI_FORMAT_R32G32_SINT,            64 },
    { RHI::RHIFormat::RG32_FLOAT,           DXGI_FORMAT_R32G32_FLOAT,           64 },
    { RHI::RHIFormat::RGB32_UINT,           DXGI_FORMAT_R32G32B32_UINT,         96 },
    { RHI::RHIFormat::RGB32_SINT,           DXGI_FORMAT_R32G32B32_SINT,         96 },
    { RHI::RHIFormat::RGB32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT,        96 },
    { RHI::RHIFormat::RGBA32_UINT,          DXGI_FORMAT_R32G32B32A32_UINT,      128 },
    { RHI::RHIFormat::RGBA32_SINT,          DXGI_FORMAT_R32G32B32A32_SINT,      128 },
    { RHI::RHIFormat::RGBA32_FLOAT,         DXGI_FORMAT_R32G32B32A32_FLOAT,     128 },
    { RHI::RHIFormat::D16,                  DXGI_FORMAT_R16_UNORM,              16 },
    { RHI::RHIFormat::D24S8,                DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  32 },
    { RHI::RHIFormat::X24G8_UINT,           DXGI_FORMAT_X24_TYPELESS_G8_UINT,   32 },
    { RHI::RHIFormat::D32,                  DXGI_FORMAT_R32_FLOAT,              32 },
    { RHI::RHIFormat::D32S8,                DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, 64 },
    { RHI::RHIFormat::X32G8_UINT,           DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  64 },
    { RHI::RHIFormat::BC1_UNORM,            DXGI_FORMAT_BC1_UNORM,              4 },
    { RHI::RHIFormat::BC1_UNORM_SRGB,       DXGI_FORMAT_BC1_UNORM_SRGB,         4 },
    { RHI::RHIFormat::BC2_UNORM,            DXGI_FORMAT_BC2_UNORM,              8 },
    { RHI::RHIFormat::BC2_UNORM_SRGB,       DXGI_FORMAT_BC2_UNORM_SRGB,         8 },
    { RHI::RHIFormat::BC3_UNORM,            DXGI_FORMAT_BC3_UNORM,              8 },
    { RHI::RHIFormat::BC3_UNORM_SRGB,       DXGI_FORMAT_BC3_UNORM_SRGB,         8 },
    { RHI::RHIFormat::BC4_UNORM,            DXGI_FORMAT_BC4_UNORM,              4 },
    { RHI::RHIFormat::BC4_SNORM,            DXGI_FORMAT_BC4_SNORM,              4 },
    { RHI::RHIFormat::BC5_UNORM,            DXGI_FORMAT_BC5_UNORM,              8 },
    { RHI::RHIFormat::BC5_SNORM,            DXGI_FORMAT_BC5_SNORM,              8 },
    { RHI::RHIFormat::BC6H_UFLOAT,          DXGI_FORMAT_BC6H_UF16,              8 },
    { RHI::RHIFormat::BC6H_SFLOAT,          DXGI_FORMAT_BC6H_SF16,              8 },
    { RHI::RHIFormat::BC7_UNORM,            DXGI_FORMAT_BC7_UNORM,              8 },
    { RHI::RHIFormat::BC7_UNORM_SRGB,       DXGI_FORMAT_BC7_UNORM_SRGB,         8 },
};

TextureCache::TextureCache(
    std::shared_ptr<IFileSystem> fs,
    RHI::IGraphicsDevice* graphicsDevice)
    : m_graphicsDevice(graphicsDevice)
    , m_fs(std::move(fs))
{}

std::shared_ptr<Assets::Texture> PhxEngine::Graphics::TextureCache::LoadTexture(
    std::shared_ptr<IBlob> textureData,
    std::string const& textureName,
    std::string const& mmeType,
    bool isSRGB,
    RHI::ICommandList* commandList)
{
    std::string cacheKey = textureName;
    std::shared_ptr<Assets::Texture> texture = this->GetTextureFromCache(cacheKey);
    if (texture)
    {
        return texture;
    }

    // Create texture
    texture = std::make_shared<Assets::Texture>();
    texture->m_forceSRGB = isSRGB;
    texture->m_path = "Blob";

    if (textureData == nullptr || IBlob::IsEmpty(*textureData))
    {
        LOG_CORE_ERROR("Failed to load texture data");
        return nullptr;
    }

    if (this->FillTextureData(*textureData, texture, "", mmeType))
    {
        this->CacheTextureData(cacheKey, texture);
        this->FinalizeTexture(texture, commandList);
    }

    return texture;
}

std::shared_ptr<Assets::Texture> PhxEngine::Graphics::TextureCache::LoadTexture(
    std::filesystem::path filename,
    bool isSRGB,
    RHI::ICommandList* commandList)
{
    std::string cacheKey = this->ConvertFilePathToKey(filename);
    std::shared_ptr<Assets::Texture> texture = this->GetTextureFromCache(cacheKey);
    if (texture)
    {
        return texture;
    }

    // Create texture
    texture = std::make_shared<Assets::Texture>();
    texture->m_forceSRGB = isSRGB;
    texture->m_path = filename.generic_string();

    std::unique_ptr<IBlob> texBlob = this->m_fs->ReadFile(texture->m_path);

    if (!texBlob || IBlob::IsEmpty(*texBlob))
    {
        LOG_CORE_ERROR("Failed to load texture data");
        return nullptr;
    }

    if (this->FillTextureData(*texBlob, texture, filename.extension().generic_string(), ""))
    {
        this->CacheTextureData(cacheKey, texture);
        this->FinalizeTexture(texture, commandList);
    }

    return texture;
}

std::shared_ptr<Assets::Texture> PhxEngine::Graphics::TextureCache::GetTexture(std::string const& key)
{
    return nullptr;
}

std::shared_ptr<Assets::Texture> PhxEngine::Graphics::TextureCache::GetTextureFromCache(std::string const& key)
{
    auto itr = this->m_loadedTextures.find(key);

    if (itr == this->m_loadedTextures.end())
    {
        return nullptr;
    }

    return itr->second;
}

bool PhxEngine::Graphics::TextureCache::FillTextureData(
    IBlob const& texBlob,
    std::shared_ptr<Assets::Texture>& texture,
	std::string const& fileExtension,
	std::string const& mimeType)
{
	HRESULT hr = S_OK;
	if (fileExtension == ".dds" || fileExtension == ".DDS" || mimeType == "image/vnd-ms.dds")
	{
		hr = LoadFromDDSMemory(
			texBlob.Data(),
			texBlob.Size(),
			DDS_FLAGS_FORCE_RGB,
			&texture->m_metadata,
            texture->m_scratchImage);
	}
	else if (fileExtension == ".hdr" || fileExtension == ".HDR")
	{
		hr = LoadFromHDRMemory(
            texBlob.Data(),
            texBlob.Size(),
			&texture->m_metadata,
            texture->m_scratchImage);
	}
	else if (fileExtension == ".tga" || fileExtension == ".TGA")
	{
		hr = LoadFromTGAMemory(
            texBlob.Data(),
            texBlob.Size(),
			&texture->m_metadata,
            texture->m_scratchImage);
	}
	else
	{
		hr = LoadFromWICMemory(
            texBlob.Data(),
            texBlob.Size(),
			WIC_FLAGS_FORCE_RGB,
			&texture->m_metadata,
            texture->m_scratchImage);
	}

	if (texture->m_forceSRGB)
	{
        texture->m_metadata.format = MakeSRGB(texture->m_metadata.format);
	}

	RHI::TextureDesc textureDesc = {};
	textureDesc.Width = texture->m_metadata.width;
	textureDesc.Height = texture->m_metadata.height;
	textureDesc.MipLevels = texture->m_metadata.mipLevels;

    for (auto& mapping : g_FormatMappings)
    {
        if (mapping.DxgiFormat == texture->m_metadata.format)
        {
            textureDesc.Format = mapping.PhxRHIFormat;
        }
    }

	textureDesc.DebugName = texture->m_path;

	switch (texture->m_metadata.dimension)
	{
	case TEX_DIMENSION_TEXTURE1D:
		textureDesc.Dimension = TextureDimension::Texture1D;
		textureDesc.ArraySize = texture->m_metadata.arraySize;
		break;

	case TEX_DIMENSION_TEXTURE2D:
		textureDesc.ArraySize = texture->m_metadata.arraySize;
		// TODO: Better handle this.
		if (textureDesc.ArraySize == 6)
		{
			textureDesc.Dimension = TextureDimension::TextureCube;
		}
		else
		{
			textureDesc.Dimension = TextureDimension::Texture2D;
		}
		break;

	case TEX_DIMENSION_TEXTURE3D:
		textureDesc.Dimension = TextureDimension::Texture3D;
		textureDesc.Depth = texture->m_metadata.depth;
		break;
	default:
		throw std::exception("Invalid texture dimension.");
		break;
	}

    // TODO
    texture->m_renderTexture = this->m_graphicsDevice->CreateTexture(textureDesc);
    // texture->Data = texBlob;

    texture->m_subresourceData.resize(texture->m_scratchImage.GetImageCount());
    const Image* pImages = texture->m_scratchImage.GetImages();
    for (int i = 0; i < texture->m_scratchImage.GetImageCount(); ++i)
    {
        SubresourceData& subresourceData = texture->m_subresourceData[i];
        subresourceData.pData = pImages[i].pixels;
        subresourceData.rowPitch = pImages[i].rowPitch;
        subresourceData.slicePitch = pImages[i].slicePitch;
    }

    return true;
}

void PhxEngine::Graphics::TextureCache::FinalizeTexture(
    std::shared_ptr<Assets::Texture>& texture,
	ICommandList* commandList)
{
    // TODO:
    commandList->TransitionBarrier(texture->m_renderTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    commandList->WriteTexture(texture->m_renderTexture, 0, texture->m_subresourceData.size(), texture->m_subresourceData.data());
    commandList->TransitionBarrier(texture->m_renderTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
}

void PhxEngine::Graphics::TextureCache::CacheTextureData(std::string key, std::shared_ptr<Assets::Texture>& texture)
{
    this->m_loadedTextures[key] = texture;
}

std::string PhxEngine::Graphics::TextureCache::ConvertFilePathToKey(std::filesystem::path const& path) const
{
    return path.generic_string();
}