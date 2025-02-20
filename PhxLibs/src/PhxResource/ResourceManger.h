#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/Span.h>

#include <unordered_map>
#include <mutex>

#include <filesystem>
#include <memory>

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

	private:
		static void RegisterPakFile(std::filesystem::path const& pakFile);

	private:
		inline static std::unique_ptr<IResourceFileSystem> ms_fileSytem;
		inline static std::unordered_map<uint32_t, RefCountPtr<IResource>> ms_cache;
		inline static std::unordered_map<uint32_t, std::unique_ptr<IResourceFactory>> ms_resourceFactories;
		inline static std::mutex ms_mutex;
	};
}

