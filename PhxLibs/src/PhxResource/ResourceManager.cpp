#include "PhxResource/PhxResource_pch.h"
#include "ResourceManager.h"

using namespace phx;

namespace
{
    // --- Internal Registry ---
    struct StoreInterface
    {
        void (*inc_ref)(GenericHandle handle);
        void (*dec_ref)(GenericHandle handle);
    };

    static std::vector<StoreInterface> ms_store_registry;

}
void ResourceManager::Initialize()
{
}

void ResourceManager::Shutdown()
{
}
void ResourceManager::RegisterStoreInterface(uint16_t id, void(*inc)(GenericHandle), void(*dec)(GenericHandle))
{
    if (id >= ms_store_registry.size()) 
        ms_store_registry.resize(id + 1);

    ms_store_registry[id] = { .inc_ref = inc, .dec_ref = dec };
}

void ResourceManager::IncRef(GenericHandle h)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].inc_ref)
            fn(h);
    }
}

void ResourceManager::DecRef(GenericHandle h)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].dec_ref)
            fn(h);
    }
}

void ResourceManager::RegisterLoader(const char* ext, IResourceFileHandler* loader) 
{ 
    ms_loaders[ext] = loader; 
}
