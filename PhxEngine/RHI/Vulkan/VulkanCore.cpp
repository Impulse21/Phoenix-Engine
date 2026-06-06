#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>

#include "RHIVulkan.h"

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>

using namespace phx::rhi::vulkan;

bool phx::rhi::Initialize(const char* app_name)
{
    PHX_LOG_INFO(Log::Channels::RHI, "Initializing RHI (Vulkan) - VkGfxDeviceImpl");

	if (volkInitialize() != VK_SUCCESS)
	{
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to initialize Volk.");
		return false;
	}

#if PHX_DEBUG
    constexpr bool use_validation_layers = true;
#else
    constexpr bool use_validation_layers = false;   
#endif

    constexpr std::array<const char*, 1> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "PhxEngine";

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = nullptr;
    instance_info.enabledExtensionCount = 0;
    instance_info.ppEnabledExtensionNames = nullptr;
    
    vulkan_check(
        vkCreateInstance(&instance_info, nullptr, &g_context.vk_instance));

    volkLoadInstance(g_context.vk_instance);
    

    return true;
}

void phx::rhi::Shutdown()
{
    vkDestroyInstance(g_context.vk_instance, nullptr);
}
