#pragma once

#include <PhxEngine/Assets/Assets.h>
#include <PhxEngine/Core/StringHash.h>
#include <memory>

#include <PhxEngine/Core/VirtualFileSystem.h>
#include <shared_mutex>
#include <atomic>
#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXTex.h>
namespace PhxEngine::Assets
{
	namespace AssetStore
	{
		void Initialize();
		void Finalize();

		std::shared_ptr<IMesh> RetrieveOrCreateMesh(PhxEngine::Core::StringHash&& hash);

		std::shared_ptr<IMaterial> RetrieveOrCreateMaterial(PhxEngine::Core::StringHash const& hash);

	}

	class TextureData : public Assets::ILoadedTexture
	{
	public:
		std::shared_ptr<Core::IBlob> Data;

		RHI::Format Format = RHI::Format::UNKNOWN;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Depth = 1;
		uint32_t ArraySize = 1;
		uint32_t MipLevels = 1;
		RHI::TextureDimension Dimension = RHI::TextureDimension::Unknown;
		bool IsRenderTarget = false;
		bool ForceSRGB = false;
		std::string Path;

		DirectX::TexMetadata Metadata;
		DirectX::ScratchImage ScratchImage;
		std::vector<RHI::SubresourceData> m_subresourceData;
	};

	class TextureAssetStore
	{
	public:
		void Reset();

		LoadedTextureHandle LoadTextureFromFileAsync(const std::filesystem::path& path, bool sRGB, Core::IFileSystem* filesystem);
		LoadedTextureHandle LoadTextureFromMemoryAsync(const std::shared_ptr<Core::IBlob>& data, std::string_view& name, const std::string& mimeType, bool sRGB);

		void UnloadTexture(LoadedTextureHandle const& texture);

		bool EnableMipmapGeneration(bool isEnabled);

	private:
		bool FindInStore(Core::StringHash hash, std::shared_ptr<TextureData>& outTexture);
		bool FillTextureData(const std::shared_ptr<Core::IBlob>& fileData, const std::shared_ptr<TextureData>& texture, const std::string& extension, const std::string& mimeType);

	private:
		std::unordered_map<uint32_t, std::shared_ptr<TextureData>> m_loadedTextureStore;
		std::shared_mutex m_loadedTextureLock;        
		std::atomic<uint32_t> m_texturesRequested = 0;
		std::atomic<uint32_t> m_texturesLoaded = 0;

	};
}

