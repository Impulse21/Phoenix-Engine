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

bool phx::rhi::Initialize(const InitParam& params)
{
    PHX_LOG_INFO(Log::Channels::RHI, "Initializing RHI (Vulkan) validation layers: {}, best practices: {}, sync validation: {}, gpu assisted: {}",
        params.enable_validation ? "ON" : "OFF",
        params.enable_best_practices ? "ON" : "OFF",
        params.enable_sync_validation ? "ON" : "OFF",
        params.enable_gpu_assisted ? "ON" : "OFF");

	if (volkInitialize() != VK_SUCCESS)
	{
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to initialize Volk.");
		return false;
	}

    constexpr std::array<const char*, 1> validation_layers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    // -- Validation layer enabled
    // TODO: Replace with allocator
    std::vector<VkValidationFeatureEnableEXT> validation_features_enabled;
    validation_features_enabled.reserve(4);

    if (params.enable_validation)
    {
        if (params.enable_best_practices)
            validation_features_enabled.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);

        if (params.enable_sync_validation)
            validation_features_enabled.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);

        if (params.enable_gpu_assisted)
        {
            validation_features_enabled.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
            validation_features_enabled.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
        }
    }

    VkValidationFeaturesEXT validation_features
    {
        .sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext                          = nullptr,
        .enabledValidationFeatureCount  = static_cast<uint32_t>(validation_features_enabled.size()),
        .pEnabledValidationFeatures     = validation_features_enabled.data(),
    };


    std::vector<const char*> instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(PHX_PLATFORM_WINDOWS)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(PHX_PLATFORM_LINUX)
        // Prefer Wayland; fall back to XCB.
        // The actual choice should come from the platform layer at runtime
        // but the extension must be baked at instance creation time.
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
    };

    if (params.enable_validation)
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = params.app_name ? params.app_name : "PhxEngine Application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "PhxEngine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    };

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = (params.enable_validation && !validation_features_enabled.empty())
                ? &validation_features
                : nullptr,
        .pApplicationInfo = &app_info,

        .enabledLayerCount = params.enable_validation
                ? static_cast<uint32_t>(validation_layers.size())
                : 0,
        .ppEnabledLayerNames = params.enable_validation
                ? validation_layers.data()
                : nullptr,

        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
        .ppEnabledExtensionNames = instance_extensions.data()
    };
    
    vulkan_check(
        vkCreateInstance(&instance_info, nullptr, &g_context.vk_instance));

    volkLoadInstance(g_context.vk_instance);

    if (params.enable_validation)
    {
        VkDebugUtilsMessengerCreateInfoEXT messager_info
        {
            .sType =
                VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

            .pfnUserCallback = DebugCallback,
            .pUserData = nullptr
        };

        vulkan_check(
            vkCreateDebugUtilsMessengerEXT(
                g_context.vk_instance,
                &messager_info,
                nullptr,
                &g_context.debug_messenger));
    }

    return true;
}

void phx::rhi::Shutdown()
{
    PHX_LOG_INFO(Log::Channels::RHI, "Shutting down RHI (Vulkan)");
    if (g_context.debug_messenger)
    {
        auto debug_messenger_fn =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(g_context.vk_instance, "vkDestroyDebugUtilsMessengerEXT"));
 
        if (debug_messenger_fn)
            debug_messenger_fn(g_context.vk_instance, g_context.debug_messenger, nullptr);
 
        g_context.debug_messenger = VK_NULL_HANDLE;
    }

    PHX_ASSERT(g_context.vk_instance != VK_NULL_HANDLE);
    vkDestroyInstance(g_context.vk_instance, nullptr);
    g_context.vk_instance = VK_NULL_HANDLE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    PHX_UNUSED(p_user_data);
    PHX_UNUSED(message_type);

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

