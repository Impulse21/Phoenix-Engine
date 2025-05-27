#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/Span.h>
#include <PhxCore/VFS.h>

#include <unordered_map>
#include <vector>
#include <mutex>

#include <filesystem>
#include <memory>

#include "PakFile.h"
#include "IAssetStreamer.h"
#include "IResource.h"
#include "IResourceHandler.h"

namespace phx
{
	class IResource;

	class ResourceManger
	{
	public:
		static void Initialize();
		static RefCountPtr<IResource> Get(std::filesystem::path const& path);

		static void RegisterPakFiles(Span<std::filesystem::path> pakFiles);
		static RefCountPtr<PakFile> RegisterPakFile(std::filesystem::path const& pakFile);
		template<typename THandler>
		static void RegisterHandler()
		{
			constexpr const char* ext = ResourceExtension<THandler>::value;
			ms_resourceHandlers[StringHash(ext).ToHash()] = std::make_unique<THandler>();
		}

		static void DrawGui();

	private:
		inline static std::shared_ptr<IAssetStreamer> ms_assetStreamer;
		inline static std::vector<RefCountPtr<PakFile>> ms_registeredPaks;
		inline static std::unordered_map<Hash32, size_t> ms_pakLut;
		inline static std::unordered_map<Hash32, RefCountPtr<IResource>> ms_cache;
		inline static std::unordered_map<Hash32, std::unique_ptr<IResourceHandler>> ms_resourceHandlers;
		inline static std::mutex ms_cacheMutex;
	};
}

