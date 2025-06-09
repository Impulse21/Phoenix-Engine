#include <PhxResource/PhxResource_pch.h>
#include "ResourceSystem.h"

#include "IResource.h"
#include <PhxCore/IO/FileUtils.h>
#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IAsyncIOSystem.h>

using namespace phx;

void phx::ResourceSystem::Initialize(data::IVirtualFileSystem* vfs, data::IAsyncIOSystem* loader)
{
	m_vfs = vfs;
	m_loader = loader;
}

void phx::ResourceSystem::Shutdown()
{
}

RefCountPtr<Resource> phx::ResourceSystem::Get(const char* virtual_file_path)
{
	// TODO: Clean up all these allocations and use simpler string functions
	// that just strip out data a single string.
	// way to many allocations probably make this code really really slow.
	StringHash filenameHash(virtual_file_path);

	{
		std::scoped_lock _(m_cacheMutex);
		auto itr = m_cache.find(filenameHash);
		if (itr != m_cache.end())
			return itr->second;
	}

	RefCountPtr<Resource> resource = nullptr;

	std::string ext = phx::GetFileExt(virtual_file_path);
	auto handlerItr = m_resourceHandlers.find(StringHash(ext));

	if (handlerItr == m_resourceHandlers.end())
	{
		PHX_CORE_ERROR("Unknown resource extension '{0}'", ext.c_str());
		return nullptr;
	}

	PHX_CORE_INFO(
		"Loading Resource '{0}' from disk",
		virtual_file_path);

	resource = handlerItr->second->LoadAsync(m_vfs, m_loader, virtual_file_path);

	if (resource)
	{
		std::scoped_lock _(m_cacheMutex);
		m_cache[filenameHash] = resource;
	}

	return resource;
}

void phx::ResourceSystem::RegisterSubResource(const std::string& virtual_path, RefCountPtr<Resource> resource)
{
	if (resource)
	{
		StringHash filenameHash(virtual_path);
		std::scoped_lock _(m_cacheMutex);
		m_cache[filenameHash] = resource;
	}
}
