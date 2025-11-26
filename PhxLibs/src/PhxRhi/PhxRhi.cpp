#include "PhxRhi_pch.h"

#include "PhxRhi.h"

#if PHX_RHI_VULKAN
#include "vulkan/VulkanBackend.h"
#include "vulkan/VulkanGpuAllocator.h"
#include "vulkan/VulkanResourceManager.h"
#include "vulkan/VulkanSubmissionManager.h"
#endif

using namespace phx;
using namespace phx::rhi;

bool rhi::Initialize(Descriptor const& /*descriptor*/, void* window_handle, size_t thread_count)
{
#if PHX_RHI_VULKAN
    VulkanBackend*              platform_backend            = new VulkanBackend(window_handle);
    VulkanResourceManager*      platform_resource_manager   = new VulkanResourceManager(platform_backend);
    VulkanSubmissionManager*    platform_submission_manager = new VulkanSubmissionManager(platform_backend, platform_resource_manager, thread_count);
#endif

    IBackend::Ptr               = platform_backend;
    IResourceManager::Ptr       = platform_resource_manager;
    ISubmissionManager::Ptr     = platform_submission_manager;

    IBackend::Ptr->Initialize();
    IResourceManager::Ptr->Initialize();
    ISubmissionManager::Ptr->Initialize();

    return true;
}


bool rhi::Shutdown()
{
    ISubmissionManager::Ptr->WaitForIdle();
    ISubmissionManager::Ptr->Shutdown();
    IResourceManager::Ptr->Shutdown();
    IBackend::Ptr->Shutdown();

    delete ISubmissionManager::Ptr;
    delete IResourceManager::Ptr;
    delete IBackend::Ptr;

    ISubmissionManager::Ptr     = nullptr;
    IResourceManager::Ptr       = nullptr;
    IBackend::Ptr               = nullptr;

    return true;
}