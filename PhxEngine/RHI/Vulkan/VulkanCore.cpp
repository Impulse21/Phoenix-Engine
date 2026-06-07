#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>

#include "RHIVulkan.h"

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>

using namespace phx::rhi::vulkan;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*,
    void*);

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

    constexpr std::array<VkValidationFeatureEnableEXT, 3> validation_enables =
    {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    };

    VkValidationFeaturesEXT validation_features
    {
        .sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext                          = nullptr,
        .enabledValidationFeatureCount  = validation_enables.size(),
        .pEnabledValidationFeatures     = validation_enables.data(),
    };

#else
    constexpr bool use_validation_layers = false;   
#endif

    constexpr const char* instance_extensions[] = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME
#if defined(PHX_PLATFORM_WINDOWS)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(PHX_PLATFORM_LINUX)
        // Prefer Wayland; fall back to XCB.
        // The actual choice should come from the platform layer at runtime
        // but the extension must be baked at instance creation time.
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#if PHX_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
 
    constexpr std::array<const char*, 1> validation_layers = 
    {
        "VK_LAYER_KHRONOS_validation"
    };
    
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "PhxEngine"
    };

    VkInstanceCreateInfo instance_info = {
        .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if PHX_DEBUG
        .pNext                      = &validation_features,
#else
        .pNext                      = nullptr,
#endif
        .pApplicationInfo           = &app_info,
        .enabledLayerCount          = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames        = validation_layers.data(),
        .enabledExtensionCount      = static_cast<uint32_t>(sizeof(instance_extensions) / sizeof(instance_extensions[0])),
        .ppEnabledExtensionNames    = instance_extensions
    };
    
    vulkan_check(
        vkCreateInstance(&instance_info, nullptr, &g_context.vk_instance));

    volkLoadInstance(g_context.vk_instance);
    
    
    if (use_validation_layers)
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerCI
        {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = DebugCallback,
            .pUserData       = nullptr,
        };
 
        auto vkCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(g_context.vk_instance, "vkCreateDebugUtilsMessengerEXT"));
 
        assert(vkCreateDebugUtilsMessengerEXT);
        vulkan_check(
            vkCreateDebugUtilsMessengerEXT(g_context.vk_instance, &messengerCI, nullptr, &g_context.debug_messenger));
    }


    return true;
}

void phx::rhi::Shutdown()
{
    if (g_context.debug_messenger)
    {
        auto vkDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(g_context.vk_instance, "vkDestroyDebugUtilsMessengerEXT"));
 
        if (vkDestroyDebugUtilsMessengerEXT)
            vkDestroyDebugUtilsMessengerEXT(g_context.vk_instance, g_context.debug_messenger, nullptr);
 
        g_context.debug_messenger = VK_NULL_HANDLE;
    }

    vkDestroyInstance(g_context.vk_instance, nullptr);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    using namespace phx::Log;
    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        PHX_LOG_TRACE(Channels::RHI, "validation layer: {}", p_callback_data->pMessage);
        break; 
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        PHX_LOG_INFO(Channels::RHI, "validation layer: {}", p_callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        PHX_LOG_WARN(Channels::RHI, "validation layer: {}", p_callback_data->pMessage);
        break;  
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        PHX_LOG_ERROR(Channels::RHI, "validation layer: {}", p_callback_data->pMessage);
        break;
    }

    return VK_FALSE;
}

