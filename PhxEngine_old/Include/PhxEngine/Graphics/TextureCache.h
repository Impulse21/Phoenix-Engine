#pragma once

#include <memory>
#include <unordered_map>
#include <queue>

#include <filesystem>

#include "PhxEngine/Scene/Assets.h"

namespace PhxEngine::Core
{
	class IFileSystem;
	class IBlob;
}
namespace PhxEngine::Graphics
{
	class TextureCache
	{
	public:
		TextureCache(
			std::shared_ptr<Core::IFileSystem> fs,
			RHI::IGraphicsDevice* graphicsDevice);

		std::shared_ptr<PhxEngine::Scene::Assets::Texture> LoadTexture(
			std::shared_ptr<Core::IBlob> textureData,
			std::string const& textureName,
			std::string const& mmeType,
			bool isSRGB,
			RHI::ICommandList* commandList);

		std::shared_ptr<PhxEngine::Scene::Assets::Texture> LoadTexture(
			std::filesystem::path filename,
			bool isSRGB,
			RHI::ICommandList* commandList);

		std::shared_ptr<PhxEngine::Scene::Assets::Texture> GetTexture(std::string const& key);

		void ClearCache() { return this->m_loadedTextures.clear(); }

	private:
		std::shared_ptr<PhxEngine::Scene::Assets::Texture> GetTextureFromCache(std::string const& key);

		bool FillTextureData(
			Core::IBlob const& texBlob,
			std::shared_ptr<PhxEngine::Scene::Assets::Texture>& texture,
			std::string const& fileExtension,
			std::string const& mimeType);

		void FinalizeTexture(
			std::shared_ptr<PhxEngine::Scene::Assets::Texture>& texture,
			RHI::ICommandList* commandList);

		void CacheTextureData(std::string key, std::shared_ptr<PhxEngine::Scene::Assets::Texture>& texture);

		std::string ConvertFilePathToKey(std::filesystem::path const& path) const;

	private:
		std::shared_ptr<Core::IFileSystem> m_fs;
		RHI::IGraphicsDevice* m_graphicsDevice;
		std::unordered_map<std::string, std::shared_ptr<PhxEngine::Scene::Assets::Texture>> m_loadedTextures;
	};
}

