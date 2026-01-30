#include "PhxResource/PhxResource_pch.h"
#include "ResourceManager.h"

using namespace phx;

void ResourceManager::Initialize()
{
    ms_async_loader = std::make_unique<AsyncLoader>();
    ms_async_loader->Start();
}

void ResourceManager::Shutdown()
{
    ms_async_loader->Stop();
}



void phx::ResourceManager::PushToGpuTransitionQueue(RefCountPtr<Resource> resource)
{
    std::scoped_lock _(ms_gpu_queue_mutex);
	ms_gpu_transition_queue.push_back(resource);
}

void phx::ResourceManager::PopPendingGpuTransitions(std::vector<RefCountPtr<Resource>>& out_batch)
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
