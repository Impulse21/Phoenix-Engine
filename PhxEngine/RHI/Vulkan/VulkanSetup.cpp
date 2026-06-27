#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StaticArray.h>

#include <PhxEngine/Memory/MemoryHelpers.h>

#include "RHIVulkan.h"
#include "RHIVulkanResources.h"

#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>

#include <thread>
#include <map>

/* Required Feature sets for Vulkan
VkPhysicalDeviceVulkan12Features.drawIndirectCountFeature flagvkCmdDrawIndexedIndirectCount — draw count from GPU buffer, essential for GPU cullingVkPhysicalDeviceVulkan12Features.samplerMirrorClampToEdgeFeature flagQuality of life, often neededVkPhysicalDeviceVulkan13Features.maintenance4Feature flagRelaxed buffer/image requirements, local workgroup size in spec constantsVK_EXT_mesh_shaderDevice ext + featureMeshlets — replaces vertex/index buffers with compute-like amplification + mesh stagesVK_EXT_multi_drawDevice ext + featureBatches multiple vkCmdDraw calls into one — cheaper than indirect for small countsVkPhysicalDeviceVulkan11Features.multiviewFeature flagStereo / shadow cascades in one passVK_EXT_shader_objectDevice ext + featurePipeline-free shader binding, pairs well with descriptor bufferVK_KHR_deferred_host_operationsDevice extRequired by ray tracing, also useful for async PSO compilationVK_KHR_acceleration_structureDevice ext + featureBLAS/TLAS — even if RT is later, worth querying support nowVK_KHR_ray_queryDevice ext + featureInline RT in any shader stage, simpler than full RT pipelineVkPhysicalDeviceVulkan12Features.timelineSemaphoreFeature flagCore 1.2 — GPU/CPU sync without fences, cleaner frame graphVkPhysicalDeviceVulkan12Features.hostQueryResetFeature flagReset timestamp pools from CPU without a command bufferVK_EXT_calibrated_timestampsDevice extCorrelate GPU timestamps to CPU clock — profilingVkPhysicalDeviceVulkan14Features.maintenance6Feature flagNull descriptor sets in bind calls, reduces validation noiseVK_AMD_anti_lag / VK_NV_low_latency2Device extVendor latency reduction — query support, enable if present
*/
using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

namespace
{
    #if false
    constexpr StaticArray<const char*, 1> device_extensions =
    {
        "VK_LAYER_KHRONOS_validation"
    };
    #endif

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
static bool InitializeVkDevice(VulkanContext& context);

static VkPhysicalDevice SelectPhysicalDevice(VkPhysicalDeviceProperties& out_properties, QueueFamilyIndices& out_queue_family_indices);
static bool GpuMeetsRequirements(VkPhysicalDevice gpu, const VkPhysicalDeviceProperties& gpu_properties);
static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice gpu);
static bool CheckDeviceExtensionSupport(VkPhysicalDevice gpu, const char* ext);
static u32 RateDeviceSuitability(VkPhysicalDevice device, const VkPhysicalDeviceProperties& gpu_props);


// -- phx RHI Implementation ----
bool phx::rhi::Initialize(const InitParam& params)
{
    g_context.allocator = params.heap_allocator;
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

    uint32_t instance_version = 0;
    vkEnumerateInstanceVersion(&instance_version);
    PHX_LOG_INFO(
        Log::Channels::RHI,
        "Vulkan Instance initialized version: {}.{}.{}",
        VK_API_VERSION_MAJOR(instance_version),
        VK_API_VERSION_MINOR(instance_version),
        VK_API_VERSION_PATCH(instance_version));

    g_context.vk_physical_device = 
        SelectPhysicalDevice(g_context.vk_physical_device_properties, g_context.queue_family_indices);

    if (g_context.vk_physical_device == VK_NULL_HANDLE)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to find a suitable Physical device.");
        return false;
    }

    if (!InitializeVkDevice(g_context))
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to initialize Vulkan device.");
        return false;
    }
    
    VkSemaphoreTypeCreateInfo timeline_type_ci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0
    };

    VkSemaphoreCreateInfo sem_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = & timeline_type_ci,
        .flags = 0
    };
    
    vulkan_check(
        vkCreateSemaphore(g_context.vk_device, &sem_create_info, NULL, &g_context.vk_timeline_sem));
        

    VkCommandPoolCreateInfo cmd_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = g_context.queue_family_indices.graphics_family.value(),
    };

    g_context.max_cmd_buffers_per_thread = params.max_cmd_buffers_per_thread;
    if (g_context.max_cmd_buffers_per_thread == 0)
    {
        g_context.max_cmd_buffers_per_thread = std::thread::hardware_concurrency();
    }
    
    for (u64 i = 0; i < VulkanContext::kMaxInflightFrames; ++i)
    {
        FrameContext& frame = g_context.frame_ctx[i];
        vulkan_check(
            vkCreateCommandPool(
                g_context.vk_device,
                &cmd_pool_ci,
                nullptr,
                &frame.vk_cmd_buffer_pool));

        std::memset(frame.vk_cmd_buffers, 0, k_max_raw_per_frame);

        frame.begin_frame_cmd_handle = rhi::CreateCommandBuffer({
            .type = CommandQueueType::Graphics
        });

        frame.end_frame_cmd_handle = rhi::CreateCommandBuffer({
            .type = CommandQueueType::Graphics
        });
    }

    return true;
}

void phx::rhi::Shutdown()
{
    PHX_LOG_INFO(Log::Channels::RHI, "Shutting down RHI (Vulkan)");

    PHX_ASSERT(g_context.vk_device != VK_NULL_HANDLE);
    vkDeviceWaitIdle(g_context.vk_device);

    g_context.deferred_callback_queue.Flush();
    
    for (u64 i = 0; i < VulkanContext::kMaxInflightFrames; ++i)
    {
        FrameContext& frame = g_context.frame_ctx[i];
        vkDestroyCommandPool(g_context.vk_device, frame.vk_cmd_buffer_pool, nullptr);
        std::memset(frame.vk_cmd_buffers, 0, k_max_raw_per_frame);

        rhi::DestoryCommandBuffer(frame.begin_frame_cmd_handle);
        rhi::DestoryCommandBuffer(frame.end_frame_cmd_handle);
    }

    vkDestroySemaphore(g_context.vk_device, g_context.vk_timeline_sem, nullptr);

    PHX_ASSERT(g_context.vma_allocator != VK_NULL_HANDLE);
    vmaDestroyAllocator(g_context.vma_allocator);

    PHX_ASSERT(g_context.vk_device != VK_NULL_HANDLE);
    vkDestroyDevice(g_context.vk_device, nullptr);

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
        .pApplicationName       = params.app_name ? params.app_name : "PhxEngine Application",
        .applicationVersion     = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName            = "PhxEngine",
        .engineVersion          = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion             = VK_API_VERSION_1_4,
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

static VkPhysicalDevice SelectPhysicalDevice(VkPhysicalDeviceProperties& out_properties, QueueFamilyIndices& out_queue_family_indices)
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

    VkPhysicalDevice best_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties best_device_props;
    u32 best_score = 0;

    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties device_props;
        vkGetPhysicalDeviceProperties(device, &device_props);

        u32 score = RateDeviceSuitability(device, device_props);
        
        PHX_LOG_INFO(
            Log::Channels::RHI,
            "Vk Physical Device '{}' scored {}.",
            device_props.deviceName,
            score);

        if (score > best_score)
        {
            best_score = score;
            best_device = device;
            best_device_props = device_props;
        }
    }

    if (best_device != VK_NULL_HANDLE)
    {
        PHX_LOG_INFO(Log::Channels::RHI,
                     "Selected VK Physical Device {} with a score of {}",
                     best_device_props.deviceName, best_score);

        out_properties = best_device_props;
        out_queue_family_indices = FindQueueFamilies(best_device);
    }

    return best_device;
}

static bool GpuMeetsRequirements(VkPhysicalDevice gpu, const VkPhysicalDeviceProperties& gpu_properties)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, exts.data());

    for (const char* ext : required_device_extensions)
    {
        if (!CheckDeviceExtensionSupport(gpu, ext))
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

    std::array<RequiredFeature, 11> required =
    {{
        { vk_features_11.multiview, "multiview" },
        { vk_features_12.bufferDeviceAddress, "bufferDeviceAddress" },
        { vk_features_12.descriptorIndexing, "descriptorIndexing" },
        { vk_features_12.drawIndirectCount, "drawIndirectCount" },
        { vk_features_12.timelineSemaphore, "timelineSemaphore" },
        { vk_features_12.hostQueryReset, "hostQueryReset" },
        { vk_features_12.samplerMirrorClampToEdge, "samplerMirrorClampToEdge" },
        { vk_features_13.dynamicRendering, "dynamicRendering" },
        { vk_features_13.synchronization2, "synchronization2" },
        { vk_features_14.maintenance6, "maintenance6" },
        { desc_buf.descriptorBuffer, "descriptorBuffer" },
    }};

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

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice gpu)
{
    PHX_ASSERT(gpu != VK_NULL_HANDLE);

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, families.data());

    QueueFamilyIndices queue_family_indices = {};

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

static bool CheckDeviceExtensionSupport(VkPhysicalDevice gpu, const char* ext)
{
    static std::vector<VkExtensionProperties> s_exts;

    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);

    s_exts.clear();
    s_exts.resize(count);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, s_exts.data());

    for (auto const& e : s_exts)
    {
        if (std::strcmp(e.extensionName, ext) == 0) 
        {
            return true;
        }   
    }

    return false;
}

static u32 RateDeviceSuitability(VkPhysicalDevice gpu, const VkPhysicalDeviceProperties& gpu_props)
{
    u32 score = 0;

    if (gpu == VK_NULL_HANDLE)
        PHX_LOG_ERROR(Log::Channels::RHI, "Unable to rate a null Physical Device");

    // Check For required Features and extensions
    if (!GpuMeetsRequirements(gpu, gpu_props))
        return 0;

    QueueFamilyIndices indices = FindQueueFamilies(gpu);
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
    VkPhysicalDeviceMeshShaderFeaturesEXT vk_mesh_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
    };

    VkPhysicalDeviceRayQueryFeaturesKHR vk_ray_query_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .pNext = &vk_mesh_features,
    };

    VkPhysicalDeviceFeatures2 vk_opt_features_2
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vk_ray_query_features,
    };
    
    vkGetPhysicalDeviceFeatures2(gpu, &vk_opt_features_2);
    
    if (vk_ray_query_features.rayQuery)
        score += 200;

    if (vk_mesh_features.meshShader)
        score += 200;

    return score;
}

static bool InitializeVkDevice(VulkanContext& context)
{
    StaticArray<VkDeviceQueueCreateInfo, 3> queue_families_ci = { };
    u32 queue_ci_count = 0;
    const float queue_priority = 1.0f;

    auto AddQueueFamily = [&](u32 family_index)
    {
        queue_families_ci[queue_ci_count++] = 
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family_index,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
    };

    AddQueueFamily(context.queue_family_indices.graphics_family.value());

    if (context.queue_family_indices.HasAsyncCompute())
    {
        AddQueueFamily(context.queue_family_indices.async_compute_family.value());
    }

    if (context.queue_family_indices.HasAsyncTransfer())
    {
        AddQueueFamily(context.queue_family_indices.async_transfer_family.value());
    }

    std::vector<const char*> device_ext =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
    };

    auto TryAddExt = [&](const char* ext, bool& cap_flag)
    {
        if (CheckDeviceExtensionSupport(context.vk_physical_device, ext))
        {
            device_ext.push_back(ext);
            cap_flag = true;
        }
        else
        {
            PHX_LOG_WARN(
                Log::Channels::RHI,
                "Optional extension {} not available",
                ext);
        }
    };

    context.capabilities = {};
    RhiCapabilities& caps = context.capabilities;

    TryAddExt(VK_EXT_MESH_SHADER_EXTENSION_NAME,              caps.mesh_shaders);
    TryAddExt(VK_KHR_RAY_QUERY_EXTENSION_NAME,                caps.ray_query);
    TryAddExt(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,   caps.acceleration_structures);
    TryAddExt(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, caps.deferred_host_operations);
    TryAddExt(VK_EXT_SHADER_OBJECT_EXTENSION_NAME,            caps.shader_object);
    TryAddExt(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,    caps.calibrated_timestamps);
    TryAddExt(VK_EXT_MULTI_DRAW_EXTENSION_NAME,               caps.multi_draw);


    VkPhysicalDeviceVulkan11Features vk_features_11
    {
        .sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext     = nullptr,
        .multiview = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features vk_features_12
    {
        .sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext                    = &vk_features_11,
        .samplerMirrorClampToEdge = VK_TRUE,
        .drawIndirectCount        = VK_TRUE,
        .descriptorIndexing       = VK_TRUE,
        .hostQueryReset           = VK_TRUE,
        .timelineSemaphore        = VK_TRUE,
        .bufferDeviceAddress      = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features vk_features_13
    {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &vk_features_12,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4     = VK_TRUE,
    };

    VkPhysicalDeviceVulkan14Features vk_features_14
    {
        .sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext          = &vk_features_13,
        .maintenance6   = VK_TRUE,
        .pushDescriptor = VK_TRUE,
    };

    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_feature
    {
        .sType                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .pNext                         = &vk_features_14,
        .descriptorBuffer              = VK_TRUE,
        .descriptorBufferCaptureReplay = VK_FALSE,
    };

    void* feature_chain_head = &descriptor_buffer_feature;

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_features
    {
        .sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
        .taskShader = VK_TRUE,
        .meshShader = VK_TRUE,
    };

    if (caps.mesh_shaders)
    {
        mesh_features.pNext = feature_chain_head;
        feature_chain_head   = &mesh_features;
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_features
    {
        .sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
    };

    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features
    {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .rayQuery = VK_TRUE,
    };

    if (caps.ray_query && caps.acceleration_structures)
    {
        accel_features.pNext        = feature_chain_head;
        ray_query_features.pNext    = &accel_features;
        feature_chain_head          = &ray_query_features;
    }

    VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_feature
    {
        .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .shaderObject = VK_TRUE,
    };

    if (caps.shader_object)
    {
        shader_object_feature.pNext = feature_chain_head;
        feature_chain_head           = &shader_object_feature;
    }

    VkPhysicalDeviceMultiDrawFeaturesEXT multi_draw_feature
    {
        .sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT,
        .multiDraw = VK_TRUE,
    };

    if (caps.multi_draw)
    {
        multi_draw_feature.pNext = feature_chain_head;
        feature_chain_head       = &multi_draw_feature;
    }

    // ---- Device create -----------------------------------------------------

    VkDeviceCreateInfo device_ci
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = feature_chain_head,
        .queueCreateInfoCount    = queue_ci_count,
        .pQueueCreateInfos       = queue_families_ci.data,
        .enabledExtensionCount   = static_cast<uint32_t>(device_ext.size()),
        .ppEnabledExtensionNames = device_ext.data(),
    };

    vulkan_check(
        vkCreateDevice(context.vk_physical_device, &device_ci, nullptr, &context.vk_device));

    volkLoadDevice(context.vk_device);

    const QueueFamilyIndices& queue_families = context.queue_family_indices;
    vkGetDeviceQueue(context.vk_device, queue_families.graphics_family.value(), 0, &context.vk_gfx_queue);
    vkGetDeviceQueue(context.vk_device, queue_families.graphics_family.value(), 0, &context.vk_present_queue);
    vkGetDeviceQueue(context.vk_device, queue_families.async_compute_family.value(), 0, &context.vk_compute_queue);
    vkGetDeviceQueue(context.vk_device, queue_families.async_transfer_family.value(), 0, &context.vk_transfer_queue);

    VmaVulkanFunctions vma_functions
    {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo vma_create_info
    {
        .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
                          | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
        .physicalDevice   = context.vk_physical_device,
        .device           = context.vk_device,
        .pVulkanFunctions = &vma_functions,
        .instance         = context.vk_instance,
        .vulkanApiVersion = VK_API_VERSION_1_4,
    };

    vulkan_check(
        vmaCreateAllocator(&vma_create_info, &context.vma_allocator));

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

