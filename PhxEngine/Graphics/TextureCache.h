#pragma once

#include <memory>
#include <unordered_map>
#include <queue>

#include <filesystem>

#include "RHI/PhxRHI.h"
namespace PhxEngine::Graphics
{
	struct LoadedTexture;
	class TextureCache
	{
	public:
		TextureCache(
			RHI::IGraphicsDevice* graphicsDevice)
			: m_graphicsDevice(graphicsDevice)
		{}

		RHI::TextureHandle LoadTexture(
			std::vector<uint8_t> const& textureData,
			std::string const& textureName,
			std::string const& mmeType,
			bool isSRGB,
			RHI::CommandListHandle commandList);

		RHI::TextureHandle LoadTexture(
			std::filesystem::path filename,
			bool isSRGB,
			RHI::CommandListHandle commandList);

		RHI::TextureHandle GetTexture(std::string const& key);

		void ClearCache() { return this->m_loadedTextures.clear(); }

	private:
		std::shared_ptr<LoadedTexture> GetTextureFromCache(std::string const& key);

		std::vector<uint8_t> ReadTextureFile(std::string const& filename);

		bool FillTextureData(
			std::vector<uint8_t> const& texBlob,
			std::shared_ptr<LoadedTexture> texture,
			std::string const& fileExtension,
			std::string const& mimeType);

		void FinalizeTexture(
			std::shared_ptr<LoadedTexture> texture,
			RHI::CommandListHandle commandList);

		void CacheTextureData(std::string key, std::shared_ptr<LoadedTexture> texture);

		std::string ConvertFilePathToKey(std::filesystem::path const& path) const;

	private:
		RHI::IGraphicsDevice* m_graphicsDevice;
		std::unordered_map<std::string, std::shared_ptr<LoadedTexture>> m_loadedTextures;
	};
}

