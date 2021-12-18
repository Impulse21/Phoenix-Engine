#include <PhxEngine/Renderer/TextureCache.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Asserts.h>

#include "DirectXTex.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

using namespace DirectX;

struct FormatMapping
{
    RHI::EFormat PhxRHIFormat;
    DXGI_FORMAT DxgiFormat;
    uint32_t BitsPerPixel;
};

const FormatMapping g_FormatMappings[] = {
    { RHI::EFormat::UNKNOWN,              DXGI_FORMAT_UNKNOWN,                0 },
    { RHI::EFormat::R8_UINT,              DXGI_FORMAT_R8_UINT,                8 },
    { RHI::EFormat::R8_SINT,              DXGI_FORMAT_R8_SINT,                8 },
    { RHI::EFormat::R8_UNORM,             DXGI_FORMAT_R8_UNORM,               8 },
    { RHI::EFormat::R8_SNORM,             DXGI_FORMAT_R8_SNORM,               8 },
    { RHI::EFormat::RG8_UINT,             DXGI_FORMAT_R8G8_UINT,              16 },
    { RHI::EFormat::RG8_SINT,             DXGI_FORMAT_R8G8_SINT,              16 },
    { RHI::EFormat::RG8_UNORM,            DXGI_FORMAT_R8G8_UNORM,             16 },
    { RHI::EFormat::RG8_SNORM,            DXGI_FORMAT_R8G8_SNORM,             16 },
    { RHI::EFormat::R16_UINT,             DXGI_FORMAT_R16_UINT,               16 },
    { RHI::EFormat::R16_SINT,             DXGI_FORMAT_R16_SINT,               16 },
    { RHI::EFormat::R16_UNORM,            DXGI_FORMAT_R16_UNORM,              16 },
    { RHI::EFormat::R16_SNORM,            DXGI_FORMAT_R16_SNORM,              16 },
    { RHI::EFormat::R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,              16 },
    { RHI::EFormat::BGRA4_UNORM,          DXGI_FORMAT_B4G4R4A4_UNORM,         16 },
    { RHI::EFormat::B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           16 },
    { RHI::EFormat::B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         16 },
    { RHI::EFormat::RGBA8_UINT,           DXGI_FORMAT_R8G8B8A8_UINT,          32 },
    { RHI::EFormat::RGBA8_SINT,           DXGI_FORMAT_R8G8B8A8_SINT,          32 },
    { RHI::EFormat::RGBA8_UNORM,          DXGI_FORMAT_R8G8B8A8_UNORM,         32 },
    { RHI::EFormat::RGBA8_SNORM,          DXGI_FORMAT_R8G8B8A8_SNORM,         32 },
    { RHI::EFormat::BGRA8_UNORM,          DXGI_FORMAT_B8G8R8A8_UNORM,         32 },
    { RHI::EFormat::SRGBA8_UNORM,         DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    32 },
    { RHI::EFormat::SBGRA8_UNORM,         DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,    32 },
    { RHI::EFormat::R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,      32 },
    { RHI::EFormat::R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        32 },
    { RHI::EFormat::RG16_UINT,            DXGI_FORMAT_R16G16_UINT,            32 },
    { RHI::EFormat::RG16_SINT,            DXGI_FORMAT_R16G16_SINT,            32 },
    { RHI::EFormat::RG16_UNORM,           DXGI_FORMAT_R16G16_UNORM,           32 },
    { RHI::EFormat::RG16_SNORM,           DXGI_FORMAT_R16G16_SNORM,           32 },
    { RHI::EFormat::RG16_FLOAT,           DXGI_FORMAT_R16G16_FLOAT,           32 },
    { RHI::EFormat::R32_UINT,             DXGI_FORMAT_R32_UINT,               32 },
    { RHI::EFormat::R32_SINT,             DXGI_FORMAT_R32_SINT,               32 },
    { RHI::EFormat::R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,              32 },
    { RHI::EFormat::RGBA16_UINT,          DXGI_FORMAT_R16G16B16A16_UINT,      64 },
    { RHI::EFormat::RGBA16_SINT,          DXGI_FORMAT_R16G16B16A16_SINT,      64 },
    { RHI::EFormat::RGBA16_FLOAT,         DXGI_FORMAT_R16G16B16A16_FLOAT,     64 },
    { RHI::EFormat::RGBA16_UNORM,         DXGI_FORMAT_R16G16B16A16_UNORM,     64 },
    { RHI::EFormat::RGBA16_SNORM,         DXGI_FORMAT_R16G16B16A16_SNORM,     64 },
    { RHI::EFormat::RG32_UINT,            DXGI_FORMAT_R32G32_UINT,            64 },
    { RHI::EFormat::RG32_SINT,            DXGI_FORMAT_R32G32_SINT,            64 },
    { RHI::EFormat::RG32_FLOAT,           DXGI_FORMAT_R32G32_FLOAT,           64 },
    { RHI::EFormat::RGB32_UINT,           DXGI_FORMAT_R32G32B32_UINT,         96 },
    { RHI::EFormat::RGB32_SINT,           DXGI_FORMAT_R32G32B32_SINT,         96 },
    { RHI::EFormat::RGB32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT,        96 },
    { RHI::EFormat::RGBA32_UINT,          DXGI_FORMAT_R32G32B32A32_UINT,      128 },
    { RHI::EFormat::RGBA32_SINT,          DXGI_FORMAT_R32G32B32A32_SINT,      128 },
    { RHI::EFormat::RGBA32_FLOAT,         DXGI_FORMAT_R32G32B32A32_FLOAT,     128 },
    { RHI::EFormat::D16,                  DXGI_FORMAT_R16_UNORM,              16 },
    { RHI::EFormat::D24S8,                DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  32 },
    { RHI::EFormat::X24G8_UINT,           DXGI_FORMAT_X24_TYPELESS_G8_UINT,   32 },
    { RHI::EFormat::D32,                  DXGI_FORMAT_R32_FLOAT,              32 },
    { RHI::EFormat::D32S8,                DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, 64 },
    { RHI::EFormat::X32G8_UINT,           DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  64 },
    { RHI::EFormat::BC1_UNORM,            DXGI_FORMAT_BC1_UNORM,              4 },
    { RHI::EFormat::BC1_UNORM_SRGB,       DXGI_FORMAT_BC1_UNORM_SRGB,         4 },
    { RHI::EFormat::BC2_UNORM,            DXGI_FORMAT_BC2_UNORM,              8 },
    { RHI::EFormat::BC2_UNORM_SRGB,       DXGI_FORMAT_BC2_UNORM_SRGB,         8 },
    { RHI::EFormat::BC3_UNORM,            DXGI_FORMAT_BC3_UNORM,              8 },
    { RHI::EFormat::BC3_UNORM_SRGB,       DXGI_FORMAT_BC3_UNORM_SRGB,         8 },
    { RHI::EFormat::BC4_UNORM,            DXGI_FORMAT_BC4_UNORM,              4 },
    { RHI::EFormat::BC4_SNORM,            DXGI_FORMAT_BC4_SNORM,              4 },
    { RHI::EFormat::BC5_UNORM,            DXGI_FORMAT_BC5_UNORM,              8 },
    { RHI::EFormat::BC5_SNORM,            DXGI_FORMAT_BC5_SNORM,              8 },
    { RHI::EFormat::BC6H_UFLOAT,          DXGI_FORMAT_BC6H_UF16,              8 },
    { RHI::EFormat::BC6H_SFLOAT,          DXGI_FORMAT_BC6H_SF16,              8 },
    { RHI::EFormat::BC7_UNORM,            DXGI_FORMAT_BC7_UNORM,              8 },
    { RHI::EFormat::BC7_UNORM_SRGB,       DXGI_FORMAT_BC7_UNORM_SRGB,         8 },
};

std::shared_ptr<Texture> PhxEngine::Renderer::TextureCache::LoadTexture(
	std::filesystem::path filename,
	bool isSRGB,
	RHI::CommandListHandle commandList)
{
    std::string cacheKey = this->ConvertFilePathToKey(filename);
    std::shared_ptr<Texture> texture = this->GetTextureFromCache(cacheKey);
    if (texture)
    {
        return texture;
    }

    // Create texture
    texture = std::make_shared<Texture>();
    texture->ForceSRGB = isSRGB;
    texture->Path = filename.generic_string();

    auto texBlob = this->ReadTextureFile(filename);

    if (!texBlob)
    {
        LOG_CORE_ERROR("Failed to load texture data");
        return nullptr;
    }

    if (this->FillTextureData(texBlob, texture, filename.extension().generic_string(), ""))
    {
        this->CacheTextureData(cacheKey, texture);
		this->FinalizeTexture(texture, commandList);
    }

    return texture;
}

std::shared_ptr<Texture> PhxEngine::Renderer::TextureCache::GetTexture(std::string const& key)
{
    return std::shared_ptr<Texture>();
}

std::shared_ptr<Texture> PhxEngine::Renderer::TextureCache::GetTextureFromCache(std::string const& key)
{
    auto itr = this->m_loadedTextures.find(key);

    if (itr == this->m_loadedTextures.end())
    {
        return nullptr;
    }

    return itr->second.lock();
}

std::shared_ptr<Core::IBlob> PhxEngine::Renderer::TextureCache::ReadTextureFile(std::filesystem::path const filename)
{
    return this->m_fileSystem->ReadFile(filename);
}

bool PhxEngine::Renderer::TextureCache::FillTextureData(
	std::shared_ptr<IBlob> texBlob,
	std::shared_ptr<Texture> texture,
	std::string const& fileExtension,
	std::string const& mimeType)
{
	TexMetadata metadata;
	ScratchImage scratchImage;

	HRESULT hr = S_OK;
	if (fileExtension == ".dds" || fileExtension == ".DDS" || mimeType == "image/vnd-ms.dds")
	{
		hr = LoadFromDDSMemory(
			texBlob->Data(),
			texBlob->Size(),
			DDS_FLAGS_FORCE_RGB,
			&metadata,
			scratchImage);
	}
	else if (fileExtension == ".hdr" || fileExtension == ".HDR")
	{
		hr = LoadFromHDRMemory(
			texBlob->Data(),
			texBlob->Size(),
			&metadata,
			scratchImage);
	}
	else if (fileExtension == ".tga" || fileExtension == ".TGA")
	{
		hr = LoadFromTGAMemory(
			texBlob->Data(),
			texBlob->Size(),
			&metadata,
			scratchImage);
	}
	else
	{
		hr = LoadFromWICMemory(
			texBlob->Data(),
			texBlob->Size(),
			WIC_FLAGS_FORCE_RGB,
			&metadata,
			scratchImage);
	}

	if (texture->ForceSRGB)
	{
		metadata.format = MakeSRGB(metadata.format);
	}

	RHI::TextureDesc textureDesc = {};
	textureDesc.Width = metadata.width;
	textureDesc.Height = metadata.height;
	textureDesc.MipLevels = metadata.mipLevels;

    for (auto& mapping : g_FormatMappings)
    {
        if (mapping.DxgiFormat == metadata.format)
        {
            textureDesc.Format = mapping.PhxRHIFormat;
        }
    }

	textureDesc.DebugName = texture->Path;

	switch (metadata.dimension)
	{
	case TEX_DIMENSION_TEXTURE1D:
		textureDesc.Dimension = TextureDimension::Texture1D;
		textureDesc.ArraySize = metadata.arraySize;
		break;

	case TEX_DIMENSION_TEXTURE2D:
		textureDesc.ArraySize = metadata.arraySize;
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
		textureDesc.Depth = metadata.depth;
		break;
	default:
		throw std::exception("Invalid texture dimension.");
		break;
	}


    texture->RhiTexture = this->m_graphicsDevice->CreateTexture(textureDesc);
    texture->Data = texBlob;

    texture->SubresourceData.resize(scratchImage.GetImageCount());
    const Image* pImages = scratchImage.GetImages();
    for (int i = 0; i < scratchImage.GetImageCount(); ++i)
    {
        SubresourceData& subresourceData = texture->SubresourceData[i];
        subresourceData.pData = pImages[i].pixels;
        subresourceData.rowPitch = pImages[i].rowPitch;
        subresourceData.slicePitch = pImages[i].slicePitch;
    }

    return false;
}

void PhxEngine::Renderer::TextureCache::FinalizeTexture(
	std::shared_ptr<Texture> texture,
	CommandListHandle commandList)
{
    commandList->TransitionBarrier(texture->RhiTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    commandList->WriteTexture(texture->RhiTexture, 0, texture->SubresourceData.size(), texture->SubresourceData.data());
    commandList->TransitionBarrier(texture->RhiTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
}

void PhxEngine::Renderer::TextureCache::CacheTextureData(std::string key, std::shared_ptr<Texture> texture)
{
    this->m_loadedTextures[key] = texture;
}

std::string PhxEngine::Renderer::TextureCache::ConvertFilePathToKey(std::filesystem::path const& path) const
{
    return path.generic_string();
}
