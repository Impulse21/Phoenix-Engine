#pragma once

#include <PhxCore/VFS.h>

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <unordered_map>
#include <mutex>

#include <filesystem>
#include <memory>

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

		static void MountPak(std::filesystem::path const& filename);
		template<typename TFactory>
		static void RegisterFactory()
		{
			constexpr const char* ext = ResourceFactoryExtension<TFactory>::value;
			ms_resourceFactories[StringHash(ext).ToHash()] = std::make_unique<TFactory>();
		}

	private:
		inline static std::unique_ptr<IRootFileSystem> ms_rootFs;
		inline static std::unordered_map<uint32_t, RefCountPtr<IResource>> ms_cache;
		inline static std::unordered_map<uint32_t, std::unique_ptr<IResourceFactory>> ms_resourceFactories;
		inline static std::mutex ms_mutex;
	};
}

