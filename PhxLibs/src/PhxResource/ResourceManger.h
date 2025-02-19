#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <unordered_map>

#include <memory>

#include "IResource.h"
#include "IResourceHandler.h"

namespace phx
{
	class IResource;

	class ResourceManger
	{
	public:
		static RefCountPtr<IResource> Get(StringHash filename, std::string const& ext);

		template<typename T>
		static void RegisterResourceHandler()
		{
			constexpr const char* ext = ResourceHandlerExtension<T>::value;
			m_resourceHandler[StringHash(ext).ToHash()] = std::make_unique<T>();
		}

	private:
		inline static std::unordered_map<uint32_t, RefCountPtr<IResource>> m_cache;
		inline static std::unordered_map<uint32_t, std::unique_ptr<IResourceHandler>> m_resourceHandler;
	};
}

