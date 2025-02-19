#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <unordered_map>
#include <mutex>

#include <memory>

#include "IResource.h"
#include "IResourceFactory.h"

namespace phx
{
	class IResource;

	class ResourceManger
	{
	public:
		static RefCountPtr<IResource> Get(const char* name, const char* ext);

		template<typename TFactory>
		static void RegisterFactory()
		{
			constexpr const char* ext = ResourceFactoryExtension<TFactory>::value;
			ms_resourceFactories[StringHash(ext).ToHash()] = std::make_unique<TFactory>();
		}

	private:
		inline static std::unordered_map<uint32_t, RefCountPtr<IResource>> ms_cache;
		inline static std::unordered_map<uint32_t, std::unique_ptr<IResourceFactory>> ms_resourceFactories;
		inline static std::mutex ms_mutex;
	};
}

