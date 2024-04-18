#include <PhxEngine/Resource/ResourceStore.h>
#include <PhxEngine/Core/Base.h>

#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/StringUtils.h>


#include <unordered_map>
#include <mutex>

using namespace PhxEngine;

namespace
{
	struct ResourceCache
	{
		size_t MemoryBudget = 0ul;
		size_t MemoryUsage = 0ul;

		std::unordered_map<StringHash, Resource*> CachedResources;
	};

	std::shared_ptr<IFileSystem> m_nativeFileSystem;
	std::vector<std::unique_ptr<IFileSystem>> m_mountedResourcePaths;
	std::unordered_map<StringHash, ResourceCache> m_resourceCache;
	std::unordered_map<StringHash, std::unique_ptr<ResourceHandler>> m_resourceHandlers;
	std::mutex m_cacheMutex;

	RefCountPtr<Resource> GetCachedResource(StringHash type, StringHash nameHash)
	{
		PHX_EVENT();
		std::scoped_lock _(m_cacheMutex);

		auto groupIter = m_resourceCache.find(type);
		if (groupIter == m_resourceCache.end())
			return nullptr;

		auto resourceItr = groupIter->second.CachedResources.find(nameHash);
		if (resourceItr == groupIter->second.CachedResources.end())
			return nullptr;

		return RefCountPtr<Resource>(resourceItr->second);
	}

	ResourceHandler* GetHandler(StringHash type)
	{
		auto itr = m_resourceHandlers.find(type);
		if (itr == m_resourceHandlers.end())
			return nullptr;

		return itr->second.get();
	}

	std::string NormalizeResourceName(std::string_view name)
	{
		return FileAccess::NormalizePath(name);
	}

	std::unique_ptr<IBlob> LoadFileFromMountedPaths(std::string const& path)
	{
		for (auto& mountedPath : m_mountedResourcePaths)
		{
			if (mountedPath->FileExists(path))
			{
				return mountedPath->ReadFile(path);
			}
		}

		return nullptr;
	}

	std::unique_ptr<IBlob> LoadFile(std::string const& path)
	{
		return LoadFileFromMountedPaths(path);
	}
}

void PhxEngine::ResourceStore::RegisterHandler(std::unique_ptr<ResourceHandler>&& handler)
{
	PHX_EVENT();
	if (GetHandler(handler->GetResourceType()))
	{
		PHX_LOG_CORE_WARN("Replacing an existing Resource Extension");
	}

	m_resourceHandlers[handler->GetResourceExt()] = std::move(handler);
}

void PhxEngine::ResourceStore::MountResourceDir(std::string const& path)
{
	MountResourceDir(path);
}

void PhxEngine::ResourceStore::MountResourceDir(std::filesystem::path const& path)
{
	if (!m_nativeFileSystem)
		m_nativeFileSystem = FileSystemFactory::CreateNativeFileSystem();

	auto mountedFileSystem = FileSystemFactory::CreateRelativeFileSystem(m_nativeFileSystem, path);
	m_mountedResourcePaths.push_back(std::move(mountedFileSystem));
}

RefCountPtr<Resource> PhxEngine::ResourceStore::GetResource(StringHash type, std::string const& name)
{
	PHX_EVENT();
	std::string nameNormalized = NormalizeResourceName(name);
	StringHash nameHash = nameNormalized.c_str();
	RefCountPtr<Resource> retVal = GetCachedResource(type, nameHash);
	if (retVal)
		return retVal;

	auto resourceHandler = GetHandler(type);
	if (!resourceHandler)
	{
		PHX_LOG_CORE_ERROR("Cound not load unknown reosurce type");
		return nullptr;
	}

	std::unique_ptr<IBlob> fileData = LoadFile(nameNormalized);
	if (!fileData)
	{
		PHX_LOG_CORE_ERROR("Unable to find resource in mounted directories '%s'", name);
		return nullptr;
	}

	retVal = resourceHandler->Load(std::move(fileData));
	if (!retVal)
	{
		PHX_LOG_CORE_ERROR("Unable to load resource '%s'", name);
		return nullptr;
	}

	retVal->AddRef();
	m_resourceCache[type].CachedResources[nameHash] = retVal;
	// Add Memory info;

	return retVal;
}

void PhxEngine::ResourceStore::RunGarbageCollection()
{
}
