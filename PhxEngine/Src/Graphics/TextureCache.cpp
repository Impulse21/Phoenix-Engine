#include "phxpch.h"

#include <PhxEngine/Graphics/TextureCache.h>
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
    RHI::FormatType PhxRHIFormat;
    DXGI_FORMAT DxgiFormat;
    uint32_t BitsPerPixel;
};

const FormatMapping g_FormatMappings[] = {
    { RHI::FormatType::UNKNOWN,              DXGI_FORMAT_UNKNOWN,                0 },
    { RHI::FormatType::R8_UINT,              DXGI_FORMAT_R8_UINT,                8 },
    { RHI::FormatType::R8_SINT,              DXGI_FORMAT_R8_SINT,                8 },
    { RHI::FormatType::R8_UNORM,             DXGI_FORMAT_R8_UNORM,               8 },
    { RHI::FormatType::R8_SNORM,             DXGI_FORMAT_R8_SNORM,               8 },
    { RHI::FormatType::RG8_UINT,             DXGI_FORMAT_R8G8_UINT,              16 },
    { RHI::FormatType::RG8_SINT,             DXGI_FORMAT_R8G8_SINT,              16 },
    { RHI::FormatType::RG8_UNORM,            DXGI_FORMAT_R8G8_UNORM,             16 },
    { RHI::FormatType::RG8_SNORM,            DXGI_FORMAT_R8G8_SNORM,             16 },
    { RHI::FormatType::R16_UINT,             DXGI_FORMAT_R16_UINT,               16 },
    { RHI::FormatType::R16_SINT,             DXGI_FORMAT_R16_SINT,               16 },
    { RHI::FormatType::R16_UNORM,            DXGI_FORMAT_R16_UNORM,              16 },
    { RHI::FormatType::R16_SNORM,            DXGI_FORMAT_R16_SNORM,              16 },
    { RHI::FormatType::R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,              16 },
    { RHI::FormatType::BGRA4_UNORM,          DXGI_FORMAT_B4G4R4A4_UNORM,         16 },
    { RHI::FormatType::B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           16 },
    { RHI::FormatType::B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         16 },
    { RHI::FormatType::RGBA8_UINT,           DXGI_FORMAT_R8G8B8A8_UINT,          32 },
    { RHI::FormatType::RGBA8_SINT,           DXGI_FORMAT_R8G8B8A8_SINT,          32 },
    { RHI::FormatType::RGBA8_UNORM,          DXGI_FORMAT_R8G8B8A8_UNORM,         32 },
    { RHI::FormatType::RGBA8_SNORM,          DXGI_FORMAT_R8G8B8A8_SNORM,         32 },
    { RHI::FormatType::BGRA8_UNORM,          DXGI_FORMAT_B8G8R8A8_UNORM,         32 },
    { RHI::FormatType::SRGBA8_UNORM,         DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    32 },
    { RHI::FormatType::SBGRA8_UNORM,         DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,    32 },
    { RHI::FormatType::R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,      32 },
    { RHI::FormatType::R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        32 },
    { RHI::FormatType::RG16_UINT,            DXGI_FORMAT_R16G16_UINT,            32 },
    { RHI::FormatType::RG16_SINT,            DXGI_FORMAT_R16G16_SINT,            32 },
    { RHI::FormatType::RG16_UNORM,           DXGI_FORMAT_R16G16_UNORM,           32 },
    { RHI::FormatType::RG16_SNORM,           DXGI_FORMAT_R16G16_SNORM,           32 },
    { RHI::FormatType::RG16_FLOAT,           DXGI_FORMAT_R16G16_FLOAT,           32 },
    { RHI::FormatType::R32_UINT,             DXGI_FORMAT_R32_UINT,               32 },
    { RHI::FormatType::R32_SINT,             DXGI_FORMAT_R32_SINT,               32 },
    { RHI::FormatType::R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,              32 },
    { RHI::FormatType::RGBA16_UINT,          DXGI_FORMAT_R16G16B16A16_UINT,      64 },
    { RHI::FormatType::RGBA16_SINT,          DXGI_FORMAT_R16G16B16A16_SINT,      64 },
    { RHI::FormatType::RGBA16_FLOAT,         DXGI_FORMAT_R16G16B16A16_FLOAT,     64 },
    { RHI::FormatType::RGBA16_UNORM,         DXGI_FORMAT_R16G16B16A16_UNORM,     64 },
    { RHI::FormatType::RGBA16_SNORM,         DXGI_FORMAT_R16G16B16A16_SNORM,     64 },
    { RHI::FormatType::RG32_UINT,            DXGI_FORMAT_R32G32_UINT,            64 },
    { RHI::FormatType::RG32_SINT,            DXGI_FORMAT_R32G32_SINT,            64 },
    { RHI::FormatType::RG32_FLOAT,           DXGI_FORMAT_R32G32_FLOAT,           64 },
    { RHI::FormatType::RGB32_UINT,           DXGI_FORMAT_R32G32B32_UINT,         96 },
    { RHI::FormatType::RGB32_SINT,           DXGI_FORMAT_R32G32B32_SINT,         96 },
    { RHI::FormatType::RGB32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT,        96 },
    { RHI::FormatType::RGBA32_UINT,          DXGI_FORMAT_R32G32B32A32_UINT,      128 },
    { RHI::FormatType::RGBA32_SINT,          DXGI_FORMAT_R32G32B32A32_SINT,      128 },
    { RHI::FormatType::RGBA32_FLOAT,         DXGI_FORMAT_R32G32B32A32_FLOAT,     128 },
    { RHI::FormatType::D16,                  DXGI_FORMAT_R16_UNORM,              16 },
    { RHI::FormatType::D24S8,                DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  32 },
    { RHI::FormatType::X24G8_UINT,           DXGI_FORMAT_X24_TYPELESS_G8_UINT,   32 },
    { RHI::FormatType::D32,                  DXGI_FORMAT_R32_FLOAT,              32 },
    { RHI::FormatType::D32S8,                DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, 64 },
    { RHI::FormatType::X32G8_UINT,           DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  64 },
    { RHI::FormatType::BC1_UNORM,            DXGI_FORMAT_BC1_UNORM,              4 },
    { RHI::FormatType::BC1_UNORM_SRGB,       DXGI_FORMAT_BC1_UNORM_SRGB,         4 },
    { RHI::FormatType::BC2_UNORM,            DXGI_FORMAT_BC2_UNORM,              8 },
    { RHI::FormatType::BC2_UNORM_SRGB,       DXGI_FORMAT_BC2_UNORM_SRGB,         8 },
    { RHI::FormatType::BC3_UNORM,            DXGI_FORMAT_BC3_UNORM,              8 },
    { RHI::FormatType::BC3_UNORM_SRGB,       DXGI_FORMAT_BC3_UNORM_SRGB,         8 },
    { RHI::FormatType::BC4_UNORM,            DXGI_FORMAT_BC4_UNORM,              4 },
    { RHI::FormatType::BC4_SNORM,            DXGI_FORMAT_BC4_SNORM,              4 },
    { RHI::FormatType::BC5_UNORM,            DXGI_FORMAT_BC5_UNORM,              8 },
    { RHI::FormatType::BC5_SNORM,            DXGI_FORMAT_BC5_SNORM,              8 },
    { RHI::FormatType::BC6H_UFLOAT,          DXGI_FORMAT_BC6H_UF16,              8 },
    { RHI::FormatType::BC6H_SFLOAT,          DXGI_FORMAT_BC6H_SF16,              8 },
    { RHI::FormatType::BC7_UNORM,            DXGI_FORMAT_BC7_UNORM,              8 },
    { RHI::FormatType::BC7_UNORM_SRGB,       DXGI_FORMAT_BC7_UNORM_SRGB,         8 },
};

std::shared_ptr<Assets::Texture> PhxEngine::Graphics::TextureCache::LoadTexture(
    std::vector<uint8_t> const& textureData,
    std::string const& textureName,
    std::string const& mmeType,
    bool isSRGB,
    RHI::CommandListHandle commandList)
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

    if (textureData.empty())
    {
        // LOG_CORE_ERROR("Failed to load texture data");
        return nullptr;
    }

    if (this->FillTextureData(textureData, texture, "", mmeType))
    {
        this->CacheTextureData(cacheKey, texture);
        this->FinalizeTexture(texture, commandList);
    }

    return texture;
}

std::shared_ptr<Assets::Texture> PhxEngine::Graphics::TextureCache::LoadTexture(
    std::filesystem::path filename,
    bool isSRGB,
    RHI::CommandListHandle commandList)
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

    auto texBlob = this->ReadTextureFile(texture->m_path);

    if (texBlob.empty())
    {
        // LOG_CORE_ERROR("Failed to load texture data");
        return nullptr;
    }

    if (this->FillTextureData(texBlob, texture, filename.extension().generic_string(), ""))
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

std::vector<uint8_t> PhxEngine::Graphics::TextureCache::ReadTextureFile(std::string const& filename)
{
    std::vector<uint8_t> retVal;
    Helpers::FileRead(filename, retVal);
    return retVal;
}

bool PhxEngine::Graphics::TextureCache::FillTextureData(
    std::vector<uint8_t> const& texBlob,
    std::shared_ptr<Assets::Texture>& texture,
	std::string const& fileExtension,
	std::string const& mimeType)
{
	HRESULT hr = S_OK;
	if (fileExtension == ".dds" || fileExtension == ".DDS" || mimeType == "image/vnd-ms.dds")
	{
		hr = LoadFromDDSMemory(
			texBlob.data(),
			texBlob.size(),
			DDS_FLAGS_FORCE_RGB,
			&texture->m_metadata,
            texture->m_scratchImage);
	}
	else if (fileExtension == ".hdr" || fileExtension == ".HDR")
	{
		hr = LoadFromHDRMemory(
			texBlob.data(),
			texBlob.size(),
			&texture->m_metadata,
            texture->m_scratchImage);
	}
	else if (fileExtension == ".tga" || fileExtension == ".TGA")
	{
		hr = LoadFromTGAMemory(
			texBlob.data(),
			texBlob.size(),
			&texture->m_metadata,
            texture->m_scratchImage);
	}
	else
	{
		hr = LoadFromWICMemory(
			texBlob.data(),
			texBlob.size(),
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
	CommandListHandle commandList)
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
