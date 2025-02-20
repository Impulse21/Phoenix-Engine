#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/Span.h>

#include <unordered_map>
#include <vector>
#include <mutex>

#include <filesystem>
#include <memory>

#include "PakFile.h"
#include "VFSResource.h"
#include "IResource.h"
#include "IResourceFactory.h"

namespace phx
{
	class IResource;
	class ResourceManger
	{
	public:
		static void Initialize(std::filesystem::path const& resourcePath);
		static RefCountPtr<IResource> Get(const char* name, const char* ext);

		static void RegisterPakFiles(Span<std::filesystem::path> pakFiles);
		template<typename TFactory>
		static void RegisterFactory()
		{
			constexpr const char* ext = ResourceFactoryExtension<TFactory>::value;
			ms_resourceFactories[StringHash(ext).ToHash()] = std::make_unique<TFactory>();
		}

		static void DrawGui();

	private:
		static void RegisterPakFile(std::filesystem::path const& pakFile);

	private:
		inline static std::vector<RefCountPtr<PakFile>> ms_registeredPaks;
		inline static std::unordered_map<Hash32, size_t> ms_pakLut;
		inline static PakFileHandler ms_pakFileHandler;
		inline static std::shared_ptr<IResourceFileSystem> ms_fileSytem;
		inline static std::unordered_map<Hash32, RefCountPtr<IResource>> ms_cache;
		inline static std::unordered_map<Hash32, std::unique_ptr<IResourceFactory>> ms_resourceFactories;
		inline static std::mutex ms_mutex;
	};
}

