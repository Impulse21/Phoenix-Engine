#include <PhxResource/PhxResource_pch.h>
#include "ResourceSystem.h"

#include "IResource.h"
#include <PhxCore/IO/FileUtils.h>

using namespace phx;

void phx::ResourceSystem::Initialize(data::IVirtualFileSystem* vfs)
{
	m_vfs = vfs;
}

void phx::ResourceSystem::Shutdown()
{
}

RefCountPtr<Resource> phx::ResourceSystem::Get(const char* path)
{
	// TODO: Clean up all these allocations and use simpler string functions
	// that just strip out data a single string.
	// way to many allocations probably make this code really really slow.
	StringHash filenameHash(path);

	{
		std::scoped_lock _(m_cacheMutex);
		auto itr = m_cache.find(filenameHash);
		if (itr != m_cache.end())
			return itr->second;
	}

	RefCountPtr<Resource> resource = nullptr;

	std::string ext = phx::GetFileExt(path);
	auto handlerItr = m_resourceHandlers.find(StringHash(ext));

	if (handlerItr == m_resourceHandlers.end())
	{
		PHX_CORE_ERROR("Unknown resource extension '{0}'", ext.c_str());
		return nullptr;
	}

	PHX_CORE_INFO(
		"Loading Resource '{0}' from disk",
		path);

	resource = handlerItr->second->LoadAsync(m_vfs, path);

	if (resource)
	{
		std::scoped_lock _(m_cacheMutex);
		m_cache[filenameHash] = resource;
	}

	return resource;
}
