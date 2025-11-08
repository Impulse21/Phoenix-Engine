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

bool rhi::Initialize(Descriptor const& descriptor, void* window_handle)
{
#if PHX_RHI_VULKAN
    VulkanBackend*              platform_backend            = new VulkanBackend(window_handle);
    VulkanGpuAllocator*         platform_gpu_allocator      = new VulkanGpuAllocator(platform_backend);
    VulkanResourceManager*      platform_resource_manager   = new VulkanResourceManager(platform_backend, platform_gpu_allocator);
    VulkanSubmissionManager*    platform_submission_manager = new VulkanSubmissionManager(platform_backend, VulkanResourceManager);
#endif

    IBackend::Ptr               = platform_backend;
    IGpuMemoryAllocator::Ptr    = platform_gpu_allocator;
    IResourceManager::Ptr       = platform_resource_manager;
    ISubmissionManager::Ptr     = platform_submission_manager;

    IBackend::Ptr->Initialize();
    IGpuMemoryAllocator::Ptr->Initialize();
    IResourceManager::Ptr->Initialize();
    ISubmissionManager::Ptr->Initialize();

    return true;
}


bool rhi::Shutdown()
{
    ISubmissionManager::Ptr->Shutdown();
    IResourceManager::Ptr->Shutdown();
    IGpuMemoryAllocator::Ptr->Shutdown();
    IBackend::Ptr->Shutdown();

    delete ISubmissionManager::Ptr;
    delete IResourceManager::Ptr;
    delete IGpuMemoryAllocator::Ptr;
    delete IBackend::Ptr;

    ISubmissionManager::Ptr     = nullptr;
    IResourceManager::Ptr       = nullptr;
    IGpuMemoryAllocator::Ptr    = nullptr;
    IBackend::Ptr               = nullptr;
}