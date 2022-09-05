#pragma once

#include <memory>
#include <unordered_map>
#include <queue>

#include <filesystem>

#include "PhxEngine/Scene/Assets.h"

namespace PhxEngine::Graphics
{
	class TextureCache
	{
	public:
		TextureCache(
			RHI::IGraphicsDevice* graphicsDevice)
			: m_graphicsDevice(graphicsDevice)
		{}

		std::shared_ptr<PhxEngine::Scene::Assets::Texture> LoadTexture(
			std::vector<uint8_t> const& textureData,
			std::string const& textureName,
			std::string const& mmeType,
			bool isSRGB,
			RHI::CommandListHandle commandList);

		std::shared_ptr<PhxEngine::Scene::Assets::Texture> LoadTexture(
			std::filesystem::path filename,
			bool isSRGB,
			RHI::CommandListHandle commandList);

		std::shared_ptr<PhxEngine::Scene::Assets::Texture> GetTexture(std::string const& key);

		void ClearCache() { return this->m_loadedTextures.clear(); }

	private:
		std::shared_ptr<PhxEngine::Scene::Assets::Texture> GetTextureFromCache(std::string const& key);

		std::vector<uint8_t> ReadTextureFile(std::string const& filename);

		bool FillTextureData(
			std::vector<uint8_t> const& texBlob,
			std::shared_ptr<PhxEngine::Scene::Assets::Texture>& texture,
			std::string const& fileExtension,
			std::string const& mimeType);

		void FinalizeTexture(
			std::shared_ptr<PhxEngine::Scene::Assets::Texture>& texture,
			RHI::CommandListHandle commandList);

		void CacheTextureData(std::string key, std::shared_ptr<PhxEngine::Scene::Assets::Texture>& texture);

		std::string ConvertFilePathToKey(std::filesystem::path const& path) const;

	private:
		RHI::IGraphicsDevice* m_graphicsDevice;
		std::unordered_map<std::string, std::shared_ptr<PhxEngine::Scene::Assets::Texture>> m_loadedTextures;
	};
}

