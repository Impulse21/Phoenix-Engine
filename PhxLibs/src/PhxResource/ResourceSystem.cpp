#include <PhxResource/PhxResource_pch.h>
#include "ResourceSystem.h"

#include "Resource.h"
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

using namespace phx;

void phx::ResourceSystem::Initialize(IVirtualFileSystem* vfs)
{
	m_vfs = vfs;
}

void phx::ResourceSystem::Shutdown()
{
	std::scoped_lock _(m_cacheMutex);
	m_cache.clear();
}
