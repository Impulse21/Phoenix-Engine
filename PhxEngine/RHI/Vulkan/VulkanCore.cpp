
#include <PhxEngine/RHI/RHI.h>

#include "RHIVulkan.h"

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>

bool phx::rhi::Initialize()
{
    return true;
}

void phx::rhi::Shutdown()
{

}
