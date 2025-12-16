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
        bool (*is_loaded)(GenericHandle);
        bool (*is_error_state)(GenericHandle);
        void (*set_state)(GenericHandle, ResourceState);
        bool (*collect_transitions)(GenericHandle handle, SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index);
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
    ms_async_loader = std::make_unique<AsyncLoader>();
    ms_async_loader->Start();
}

void ResourceManager::Shutdown()
{
    ms_async_loader->Stop();
}

void ResourceManager::RegisterStoreInterface(
    uint16_t id,
    void(*inc)(GenericHandle),
    void(*dec)(GenericHandle),
    bool(*is_loaded)(GenericHandle),
    bool (*is_error_state)(GenericHandle),
    void(*set_state)(GenericHandle, ResourceState),
    bool(*collect_transitions)(GenericHandle, SpanMutable<rhi::GpuBarrier>, size_t&))
{
    if (id >= ms_store_registry.size()) 
        ms_store_registry.resize(id + 1);

    ms_store_registry[id] = { 
        .inc_ref = inc,
        .dec_ref = dec,
		.is_loaded = is_loaded,
		.is_error_state = is_error_state,
        .set_state = set_state,
        .collect_transitions = collect_transitions
    };
}

bool phx::ResourceManager::IsLoaded(GenericHandle h)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].is_loaded)
            return fn(h);
    }

    return false;
}

bool phx::ResourceManager::IsErrorState(GenericHandle h)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].is_error_state)
            return fn(h);
    }

    return false;
}

void phx::ResourceManager::SetState(GenericHandle h, ResourceState state)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].set_state)
            fn(h, state);
    }
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


bool ResourceManager::CollectPendingGpuTransitions(GenericHandle h, SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index)
{
    if (h.type_id < ms_store_registry.size())
    {
        if (auto fn = ms_store_registry[h.type_id].collect_transitions)
            return fn(h, transitions, fill_index);
    }

    return false;
}
