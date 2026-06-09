#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StaticArray.h>

#include "RHIVulkan.h"

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>

#include <map>

/* Required Feature sets for Vulkan
VkPhysicalDeviceVulkan12Features.drawIndirectCountFeature flagvkCmdDrawIndexedIndirectCount — draw count from GPU buffer, essential for GPU cullingVkPhysicalDeviceVulkan12Features.samplerMirrorClampToEdgeFeature flagQuality of life, often neededVkPhysicalDeviceVulkan13Features.maintenance4Feature flagRelaxed buffer/image requirements, local workgroup size in spec constantsVK_EXT_mesh_shaderDevice ext + featureMeshlets — replaces vertex/index buffers with compute-like amplification + mesh stagesVK_EXT_multi_drawDevice ext + featureBatches multiple vkCmdDraw calls into one — cheaper than indirect for small countsVkPhysicalDeviceVulkan11Features.multiviewFeature flagStereo / shadow cascades in one passVK_EXT_shader_objectDevice ext + featurePipeline-free shader binding, pairs well with descriptor bufferVK_KHR_deferred_host_operationsDevice extRequired by ray tracing, also useful for async PSO compilationVK_KHR_acceleration_structureDevice ext + featureBLAS/TLAS — even if RT is later, worth querying support nowVK_KHR_ray_queryDevice ext + featureInline RT in any shader stage, simpler than full RT pipelineVkPhysicalDeviceVulkan12Features.timelineSemaphoreFeature flagCore 1.2 — GPU/CPU sync without fences, cleaner frame graphVkPhysicalDeviceVulkan12Features.hostQueryResetFeature flagReset timestamp pools from CPU without a command bufferVK_EXT_calibrated_timestampsDevice extCorrelate GPU timestamps to CPU clock — profilingVkPhysicalDeviceVulkan14Features.maintenance6Feature flagNull descriptor sets in bind calls, reduces validation noiseVK_AMD_anti_lag / VK_NV_low_latency2Device extVendor latency reduction — query support, enable if present
*/
using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

namespace
{
    constexpr StaticArray<const char*, 1> device_extensions =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr StaticArray<const char*, 2> required_device_extensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
    };
}

// -- Forward Declares ----
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*,
    void*);

static bool InitializeVkInstance(const InitParam& params, VulkanContext& context);
static VkPhysicalDevice SelectPhysicalDevice(const InitParam& params, VulkanContext::QueueFamilyIndices& queue_family_indices);
static bool GpuMeetsRequirements(VkPhysicalDevice gpu, const VkPhysicalDeviceProperties& gpu_properties);
static VulkanContext::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice gpu);
static u32 RateDeviceSuitability(VkPhysicalDevice device);

static bool InitializeVkDevice(const InitParam& params, VulkanContext& context);

// -- phx RHI Implementation ----
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

    if (!InitializeVkInstance(params, g_context))
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to initialize Vulkan instance.");
        return false;
    }

    if (!InitializeVkDevice(params, g_context))
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to initialize Vulkan device.");
        return false;
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


static bool InitializeVkInstance(const InitParam& params, VulkanContext& context)
{
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
        vkCreateInstance(&instance_info, nullptr, &context.vk_instance));

    volkLoadInstance(context.vk_instance);

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
                context.vk_instance,
                &messager_info,
                nullptr,
                &context.debug_messenger));
    }

    return true;
}

static VkPhysicalDevice SelectPhysicalDevice(const InitParam& params, VulkanContext::QueueFamilyIndices& queue_family_indices)
{
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(g_context.vk_instance, &device_count, nullptr);

    if (device_count == 0)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to find any physical devices with Vulkan support.");
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(g_context.vk_instance, &device_count, devices.data());

    std::multimap<u32, VkPhysicalDevice> candidates;

    for (const auto& device : devices)
    {
        u32 score = RateDeviceSuitability(device);
        candidates.insert({ score, device });
    }

    return candidates.empty() ? VK_NULL_HANDLE : candidates.rbegin()->second;
}

static bool GpuMeetsRequirements(VkPhysicalDevice gpu, const VkPhysicalDeviceProperties& gpu_properties)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, exts.data());

    for (const char* ext : required_device_extensions)
    {
        bool ext_supported = false;
        for (auto const& e : exts)
        {
            if (std::strcmp(e.extensionName, ext) == 0) 
            {
                ext_supported = true;
            }
        }

        if (!ext_supported)
        {
            PHX_LOG_WARN(Log::Channels::RHI, "Vulkan device '{}' is missing required extension: {}", gpu_properties.deviceName, ext);
            return false;
        }
    }

    VkPhysicalDeviceVulkan11Features vk_features_11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features vk_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &vk_features_11};
    VkPhysicalDeviceVulkan13Features vk_features_13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &vk_features_12};
    VkPhysicalDeviceVulkan14Features vk_features_14{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = &vk_features_13};

    VkPhysicalDeviceDescriptorBufferFeaturesEXT desc_buf{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .pNext = &vk_features_14,
    };

    VkPhysicalDeviceFeatures2 vk_features_2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &desc_buf,
    };

    vkGetPhysicalDeviceFeatures2(gpu, &vk_features_2);

    // Required — device is unusable without these.
    struct RequiredFeature
    {
        VkBool32 supported;
        const char* name;
    };

    StaticArray<RequiredFeature, 8> required = {
        {vk_features_11.multiview, "multiview"},
        {vk_features_12.bufferDeviceAddress, "bufferDeviceAddress"},
        {vk_features_12.descriptorIndexing, "descriptorIndexing"},
        {vk_features_12.drawIndirectCount, "drawIndirectCount"},
        {vk_features_12.timelineSemaphore, "timelineSemaphore"},
        {vk_features_12.hostQueryReset, "hostQueryReset"},
        {vk_features_12.samplerMirrorClampToEdge, "samplerMirrorClampToEdge"},
        {vk_features_13.dynamicRendering, "dynamicRendering"},
        {vk_features_13.synchronization2, "synchronization2"},
        {vk_features_14.maintenance6, "maintenance6"},
        {desc_buf.descriptorBuffer, "descriptorBuffer"},
    };

    for (auto const& r : required)
    {
        if (!r.supported)
        {
            PHX_LOG_WARN(
                Log::Channels::RHI,
                "Vulkan device '{}' rejected — missing required feature: {}",
                gpu_properties.deviceName, r.name);

            return false;
        }
    }

    return true;
}

VulkanContext::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice gpu)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, families.data());

    VulkanContext::QueueFamilyIndices queue_family_indices = {};

    for (uint32_t i = 0; i < count; ++i)
    {
        const VkQueueFlags flags = families[i].queueFlags;

        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_family_indices.graphics_family = i;
        }

        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            queue_family_indices.async_compute_family = i;
        }

        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            queue_family_indices.async_transfer_family = i;
        }
    }

    // Fallbacks — resolved here so callers always get valid indices.
    // Logged at INFO so it's visible when running on integrated hardware.
    if (queue_family_indices.async_compute_family.has_value() == false)
        queue_family_indices.async_compute_family = queue_family_indices.graphics_family;

    if (queue_family_indices.async_transfer_family.has_value() == false)
        queue_family_indices.async_transfer_family = queue_family_indices.graphics_family;

    return queue_family_indices;
}

static u32 RateDeviceSuitability(VkPhysicalDevice gpu)
{
    u32 score = 0;

    VkPhysicalDeviceProperties gpu_props;
    vkGetPhysicalDeviceProperties(gpu, &gpu_props);
    
    // Check For required Features and extensions
    if (!GpuMeetsRequirements(gpu, gpu_props))
        return 0;

    VulkanContext::QueueFamilyIndices indices = FindQueueFamilies(gpu);
    if (!indices.IsComplete())
    {
        PHX_LOG_WARN(Log::Channels::RHI, "Vulkan device '{}' rejected — missing graphics queue family.", gpu_props.deviceName);
        return 0;
    }

    if (indices.HasAsyncCompute())
    {
        score += 500;
    }

    if (indices.HasAsyncTransfer())
    {
        score += 500;
    }

    if (gpu_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
    {
        score += 1000;
    }

    score += gpu_props.limits.maxImageDimension2D;

    // Optional features — present is better, absent is still usable.
    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
    };

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .pNext = &meshFeatures,
    };

    VkPhysicalDeviceFeatures2 optFeatures2
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &rayQueryFeatures,
    };

    // Only query optional extension features if the extensions are present,
    // otherwise the pNext chain entry is ignored by the driver anyway, but
    // it avoids any loader warnings on strict drivers.
    if (CheckDeviceExtensionSupport(gpu, VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
        CheckDeviceExtensionSupport(gpu, VK_EXT_MESH_SHADER_EXTENSION_NAME))
    {
        vkGetPhysicalDeviceFeatures2(gpu, &optFeatures2);

        if (rayQueryFeatures.rayQuery)
            score += 200;

        if (meshFeatures.meshShader)
            score += 200;
    }
    
    return score;
}

static bool InitializeVkDevice(const InitParam& params, VulkanContext& context)
{
    return true;
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

