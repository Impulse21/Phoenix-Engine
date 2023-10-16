#include <PhxEngine/Assets/AssetStore.h>

#include <PhxEngine/Engine/EngineCore.h>
#include <unordered_map>
#include <mutex>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Assets;

namespace
{

	std::unordered_map<uint32_t, std::weak_ptr<IMesh>> m_meshStore;
	std::mutex m_meshLock;


	std::unordered_map<uint32_t, std::weak_ptr<IMaterial>> m_materialStore;
	std::mutex m_materailLock;

	template<typename T>
	std::shared_ptr<T> RetrieveOrCreateAsset(StringHash const& hash, std::unordered_map<uint32_t, std::weak_ptr<T>>& assetStore)
	{
		auto itr = assetStore.find(hash.computedHash);
		std::shared_ptr<T> retVal = nullptr;
		if (itr != assetStore.end())
		{
			if (retVal = itr->second.lock())
			{
				return retVal;
			}
		}

		// WE need to create a new assets
		retVal = std::make_shared<T>();
		assetStore[hash.computedHash] = retVal;

		return retVal;
	}
}

void AssetStore::Initialize()
{
}

void AssetStore::Finalize()
{
	m_meshStore.clear();
	m_materialStore.clear();
}

std::shared_ptr<IMesh> PhxEngine::Assets::AssetStore::RetrieveOrCreateMesh(PhxEngine::Core::StringHash&& hash)
{
	std::scoped_lock _(m_meshLock);
	return RetrieveOrCreateAsset<IMesh>(hash, m_meshStore);
}

std::shared_ptr<IMaterial> PhxEngine::Assets::AssetStore::RetrieveOrCreateMaterial(PhxEngine::Core::StringHash const& hash)
{
	std::scoped_lock _(m_materailLock);
	return RetrieveOrCreateAsset<IMaterial>(hash, m_materialStore);
}


void PhxEngine::Assets::TextureAssetStore::Reset()
{
	this->m_loadedTextureStore.clear();
	this->m_texturesRequested = 0;
	this->m_texturesLoaded = 0;
}

LoadedTextureHandle PhxEngine::Assets::TextureAssetStore::LoadTextureFromFileAsync(const std::filesystem::path& path, bool sRGB, Core::IFileSystem* filesystem)
{
	std::shared_ptr<TextureData> loadedTexture;
	if (this->FindInStore(path.generic_string().c_str(), loadedTexture))
	{
		return loadedTexture;
	}

	loadedTexture->ForceSRGB = sRGB;
	loadedTexture->Path = path.generic_string();

	PhxEngine::GetTaskExecutor().async([this, sRGB, loadedTexture, path, filesystem]() {
			std::shared_ptr<IBlob> fileData = filesystem->ReadFile(path);
			if (fileData)
			{
				if (FillTextureData(fileData, loadedTexture, path.extension().generic_string(), ""))
				{
					// LOG Texture Loaded
					// TODO: I am here
					std::lock_guard<std::mutex> guard(m_TexturesToFinalizeMutex);

					m_TexturesToFinalize.push(texture);
				}
			}

			++this->m_texturesLoaded;
		});

	return loadedTexture;
}

bool PhxEngine::Assets::TextureAssetStore::FindInStore(Core::StringHash hash, std::shared_ptr<TextureData>& outTexture)
{
	std::scoped_lock _(this->m_loadedTextureLock);

	// First see if this texture is already loaded (or being loaded).

	outTexture = this->m_loadedTextureStore[hash];
	if (outTexture)
	{
		return true;
	}

	// Allocate a new texture slot for this file name and return it. Load the file later in a thread pool.
	// LoadTextureFromFileAsync function for a given scene is only called from one thread, so there is no 
	// chance of loading the same texture twice.

	outTexture = std::make_shared<TextureData>();;
	this->m_loadedTextureStore[hash] = outTexture;

	++this->m_texturesRequested;

	return false;
}

bool PhxEngine::Assets::TextureAssetStore::FillTextureData(
	std::shared_ptr<Core::IBlob> const& fileData,
	const std::shared_ptr<Assets::TextureData>& texture,
	const std::string& extension,
	const std::string& mimeType)
{
	HRESULT hr = S_OK;
	if (extension == ".dds" || extension == ".DDS" || mimeType == "image/vnd-ms.dds")
	{
		texture->Data = fileData;

		hr = DirectX::LoadFromDDSMemory(
			fileData->Data(),
			fileData->Size(),
			texture->ForceSRGB ? DirectX::DDS_FLAGS_FORCE_RGB : DirectX::DDS_FLAGS_NONE,
			&texture->Metadata,
			texture->ScratchImage);

	}
	else if (extension == ".hdr" || extension == ".HDR")
	{
		hr = DirectX::LoadFromHDRMemory(
			fileData->Data(),
			fileData->Size(),
			&texture->Metadata,
			texture->ScratchImage);
	}
	else if (extension == ".tga" || extension == ".TGA")
	{
		hr = DirectX::LoadFromTGAMemory(
			fileData->Data(),
			fileData->Size(),
			&texture->Metadata,
			texture->ScratchImage);
	}
	else
	{
		hr = DirectX::LoadFromWICMemory(
			fileData->Data(),
			fileData->Size(),
			texture->ForceSRGB ? DirectX::WIC_FLAGS_FORCE_RGB : DirectX::WIC_FLAGS_NONE,
			&texture->Metadata,
			texture->ScratchImage);
	}

	if (!FAILED(hr))
	{
		texture->Data = nullptr;
		PHX_LOG_CORE_INFO("Couldn't load texture '%s'", texture->Path.c_str());
		return false;
	}
    return true;
}
