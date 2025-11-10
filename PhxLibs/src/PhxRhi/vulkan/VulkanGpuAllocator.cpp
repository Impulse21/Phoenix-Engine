#include "PhxRhi/PhxRhi_pch.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include "VulkanGpuAllocator.h"

#include "VulkanBackend.h"

bool phx::rhi::VulkanGpuAllocator::Initialize()
{
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = vulkan_backend->vk_chosen_physical_device;
    allocator_info.device = vulkan_backend->vk_device;
    allocator_info.instance = vulkan_backend->vk_instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

    allocator_info.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
        VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

    // VkPhysicalDeviceFeatures enabled_features; // Not needed, using vulkan_backend->vk_features_1_2 directly
    // vkGetPhysicalDeviceFeatures(vulkan_backend->vk_chosen_physical_device, &enabled_features); // Example, better to use vkb info

    if (vulkan_backend->vk_features_1_2.bufferDeviceAddress)
    {
        allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }

    VkResult res = vmaCreateAllocator(&allocator_info, &vma_allocator);
    if (res != VK_SUCCESS)
    {
        PHX_RHI_ERROR("Failed to create VMA Allocator. VkResult: <TODO>");
        return false;
    }

    const VkPhysicalDeviceMemoryProperties* memory_properties;
    vmaGetMemoryProperties(vma_allocator, &memory_properties);

    for (uint32_t i = 0; i < memory_properties->memoryHeapCount; i++)
    {
        const VkMemoryHeap& heap = memory_properties->memoryHeaps[i];

        if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
        {
            for (uint32_t j = 0; j < memory_properties->memoryTypeCount; j++)
            {
                const VkMemoryType& memory_type = memory_properties->memoryTypes[j];
                if (memory_type.heapIndex == i)
                {
                    if ((memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
                    {
                        vulkan_backend->vk_rebar_heap_size = heap.size;
                        PHX_RHI_INFO("Rebar Heap found {0}", PhxToMB(vulkan_backend->vk_rebar_heap_size));
                        break;
                    }
                }
            }
        }
    }

    return true;
}

void phx::rhi::VulkanGpuAllocator::Shutdown()
{
    if (vma_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(vma_allocator);
        vma_allocator = VK_NULL_HANDLE;
    }
}

phx::rhi::VulkanGpuAllocator::VulkanGpuAllocator(VulkanBackend* vulkan_backend)
    : vulkan_backend(vulkan_backend)
{
}
