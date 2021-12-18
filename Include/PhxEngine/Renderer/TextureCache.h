#pragma once

#include <memory>
#include <unordered_map>
#include <queue>

#include <filesystem>
#include <PhxEngine/Core/FileSystem.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/SceneDataTypes.h>

namespace PhxEngine::Renderer
{
	class TextureCache
	{
	public:
		TextureCache(
			RHI::IGraphicsDevice* graphicsDevice,
			std::shared_ptr<Core::IFileSystem> fileSystem)
			: m_graphicsDevice(graphicsDevice)
			, m_fileSystem(fileSystem)
		{}

		std::shared_ptr<Texture> LoadTexture(
			std::shared_ptr<Core::IBlob> textureData,
			std::string const& textureName,
			std::string const& mmeType,
			bool isSRGB,
			RHI::CommandListHandle commandList) {};

		std::shared_ptr<Texture> LoadTexture(
			std::filesystem::path filename,
			bool isSRGB,
			RHI::CommandListHandle commandList);

		std::shared_ptr<Texture> GetTexture(std::string const& key);

	private:
		std::shared_ptr<Texture> GetTextureFromCache(std::string const& key);

		std::shared_ptr<Core::IBlob> ReadTextureFile(std::filesystem::path const filename);

		bool FillTextureData(
			std::shared_ptr<Core::IBlob> texBlob,
			std::shared_ptr<Texture> texture,
			std::string const& fileExtension,
			std::string const& mimeType);

		void FinalizeTexture(
			std::shared_ptr<Texture> texture,
			RHI::CommandListHandle commandList);

		void CacheTextureData(std::string key, std::shared_ptr<Texture> texture);

		std::string ConvertFilePathToKey(std::filesystem::path const& path) const;

	private:
		std::shared_ptr<Core::IFileSystem> m_fileSystem;
		RHI::IGraphicsDevice* m_graphicsDevice;

		std::unordered_map<std::string, std::weak_ptr<Texture>> m_loadedTextures;
		std::queue<std::shared_ptr<Texture>> m_pendingFinalization;
	};
}

