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
        bool (*collect_transitions)(GenericHandle handle, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index);
    };

    static std::vector<StoreInterface> ms_store_registry;

}

void phx::ResourceIncRef(GenericHandle h)
{
	ResourceManager::IncRef(h);
}

void phx::ResourceDecRef(GenericHandle h)
{
	ResourceManager::DecRef(h);
}


void ResourceManager::Initialize()
{
}

void ResourceManager::Shutdown()
{
}

void ResourceManager::RegisterStoreInterface(
    uint16_t id,
    void(*inc)(GenericHandle),
    void(*dec)(GenericHandle),
    bool(*collect_transitions)(GenericHandle, SpanMutable<GpuTransitionWork>, size_t&))
{
    if (id >= ms_store_registry.size()) 
        ms_store_registry.resize(id + 1);

    ms_store_registry[id] = { 
        .inc_ref = inc,
        .dec_ref = dec,
        .collect_transitions = collect_transitions
    };
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

void phx::ResourceManager::PushToGpuTransitionQueue(GenericHandle handle)
{
    std::scoped_lock _(ms_gpu_queue_mutex);
	ms_gpu_transition_queue.push_back(handle);
}

void phx::ResourceManager::PopPendingGpuTransitions(std::vector<GenericHandle>& out_batch)
{
    std::scoped_lock _(ms_gpu_queue_mutex);
    if (ms_gpu_transition_queue.empty()) 
        return;

    out_batch.reserve(out_batch.size() + ms_gpu_transition_queue.size());
    out_batch.insert(
        out_batch.end(),
        ms_gpu_transition_queue.begin(),
        ms_gpu_transition_queue.end());

    ms_gpu_transition_queue.clear();
}


bool ResourceManager::CollectPendingGpuTransitions(GenericHandle h, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].collect_transitions)
            return fn(h, transitions, fill_index);
    }

    return false;
}
