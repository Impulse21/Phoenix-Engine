#include <PhxEngine/Resource/ResourceLoader.h>
#include <PhxEngine/Core/Base.h>

#include <unordered_map>
#include <PhxEngine/Core/VirtualFileSystem.h>

using namespace PhxEngine;

namespace
{
	std::unordered_map<StringHash, std::unique_ptr<ResourceHandler>> m_resourceHandlers;

	ResourceHandler* GetLoader(StringHash hash)
	{
		auto itr = m_resourceHandlers.find(hash);
		if (itr == m_resourceHandlers.end())
			return nullptr;

		return itr->second.get();
	}
}

bool PhxEngine::ResourceLoader::RegonizedPath(std::string_view path)
{
	PHX_EVENT();
	return GetLoader(FileAccess::GetFileExtensionId(path));
}

void PhxEngine::ResourceLoader::Exists(std::string const& exists)
{
	PHX_EVENT();
}

RefCountPtr<Resource> PhxEngine::ResourceLoader::Load(std::string const& path)
{
	PHX_EVENT();
	ResourceHandler* handler = GetLoader(path.c_str());
	if (!handler)
	{
		PHX_LOG_CORE_ERROR("Unable to locate a resource handler for '%s'", path.c_str());
		return nullptr;
	}

	// Begin Loading the Asset
	return handler->Load(path);
}

void PhxEngine::ResourceLoader::RegisterHandler(std::unique_ptr<ResourceHandler>&& handler)
{
	PHX_EVENT();
	if (GetLoader(handler->GetResourceExt()))
	{
		PHX_LOG_CORE_WARN("Replacing an existing Resource Extension");
	}

	m_resourceHandlers[handler->GetResourceExt()] = std::move(handler);
}
