#include "PhxResource/PhxResource_pch.h"

#include "PhxCore/Base.h"
#include "ResourceManger.h"

using namespace phx;

RefCountPtr<IResource> ResourceManger::Get(const char* name, const char* ext)
{
	StringHash filenameHash(std::format("{}.{}", name, ext));

	std::scoped_lock _(ms_mutex);
	auto itr = ms_cache.find(filenameHash);
	if (itr != ms_cache.end())
		return itr->second;

	auto factoryItr = ms_resourceFactories.find(StringHash(ext));
	if (factoryItr == ms_resourceFactories.end())
	{
		PHX_CORE_ERROR("Unknown File extension encounters: '{0}'", ext);
		return nullptr;
	}

	ms_cache[filenameHash] = factoryItr->second->Create(filenameHash, name);
	ms_cache[filenameHash]->StartMetadataLoad();
	return ms_cache[filenameHash];
}