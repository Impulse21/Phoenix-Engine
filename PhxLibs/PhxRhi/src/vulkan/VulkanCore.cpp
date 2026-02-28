#include "PhxRhi_pch.h"
#include <PhxRhi/PhxRhi.h>

#include <PhxCore/Platform/PlatformConfig.h>

#include "VulkanInternal.h"


#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-private-field"

#endif


#if defined(PHX_PLATFORM_WINDOWS)

#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
#define VK_USE_PLATFORM_WIN32_KHR

#else

#include <wayland-client.h>
#define VK_USE_PLATFORM_WAYLAND_KHR

#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"


#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "PhxCore/Math.h"


#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_RHI_WARN("[Vulkan] - Pool '" #x "' still contains active handles"); x.Shutdown();

inline static VKAPI_ATTR VkBool32 VKAPI_CALL vk_phx_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void*);


namespace
{
    constexpr size_t kMaxNumSwapchains = 1;
    constexpr size_t kMaxPipelineStates = 200;
    constexpr size_t kMaxNumBuffers = 4096;
    constexpr size_t kMaxNumTextures = 4096;
}

namespace phx::rhi::vulkan
{
    bool SelectPhysicalDevice(vkb::PhysicalDevice&);
    bool CreateLogicalDevice(vkb::PhysicalDevice&);
    bool InitVma();
    bool CreateSurface(void*);
}

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

bool phx::rhi::Initialize(Descriptor const& descriptor, void* window_handle, size_t thread_count)
{
    PHX_PROFILE_SECTION("Vulkan::PlatformInitialize");
    PHX_RHI_INFO("Initializing RHI (Vulkan) - VkGfxDeviceImpl");

	if (volkInitialize() != VK_SUCCESS)
	{
		PHX_RHI_ERROR("[RHI] Failed to initialize Volk.");
		return false;
	}

#if PHX_DEBUG
    bool use_validation_layers = true;
#else
    bool use_validation_layers = false;
#endif

    vkb::InstanceBuilder builder;
    builder.set_app_name("Phoenix RHI Application")
        .set_engine_name("PhxEngine")
        .request_validation_layers(use_validation_layers)
        .set_debug_callback(vk_phx_debug_callback)
        .set_headless(false)
        .require_api_version(1, 3, 0);

    auto inst_ret = builder.build();
    if (!inst_ret)
    {
        PHX_RHI_ERROR("Failed to create Vulkan Instance: {0}", inst_ret.error().message());
        return false;
    }

    g_vulkan.vkb_instance = inst_ret.value();
    g_vulkan.vk_instance = g_vulkan.vkb_instance.instance;
    g_vulkan.vk_debug_messenger = g_vulkan.vkb_instance.debug_messenger;
    volkLoadInstance(g_vulkan.vk_instance);

    if (!vulkan::CreateSurface(window_handle))
    {
        vkb::destroy_instance(g_vulkan.vkb_instance);
        return false;
    }

    vkb::PhysicalDevice vkb_physical_device; // Renamed to snake_case
    if (!vulkan::SelectPhysicalDevice(vkb_physical_device))
    {
        vkDestroySurfaceKHR(g_vulkan.vk_instance, g_vulkan.vk_surface, nullptr);
        vkb::destroy_instance(g_vulkan.vkb_instance);
        return false;
    }

    if (!vulkan::CreateLogicalDevice(vkb_physical_device))
    {
        vkDestroySurfaceKHR(g_vulkan.vk_instance, g_vulkan.vk_surface, nullptr);
        vkb::destroy_instance(g_vulkan.vkb_instance);
        return false;
    }

    vulkan::InitVma();
    g_vulkan.descriptor_system.Initialize(
        g_vulkan.vk_device,
        g_vulkan.vma_allocator,
        g_vulkan.vk_chosen_physical_device);

    g_vulkan.swapchain_pool.Initialize(kMaxNumSwapchains);
    g_vulkan.buffer_pool.Initialize(kMaxNumBuffers);
    g_vulkan.texture_pool.Initialize(kMaxNumTextures);
    g_vulkan.pipeline_state_pool.Initialize(kMaxPipelineStates);
    g_vulkan.shader_module_pool.Initialize(kMaxPipelineStates * 2);

    VkPipelineCacheCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    vulkan_check(
        vkCreatePipelineCache(g_vulkan.vk_device, &createInfo, nullptr, &g_vulkan.vk_pipeline_cache));

    g_vulkan.submission.Initialize(thread_count);

    g_vulkan.dynamic_upload_ring.Initialize(g_vulkan.vk_device, g_vulkan.vma_allocator, kDynamicBufferSize, kDynamicBufferBlockSize);
	return false;
}

void phx::rhi::Shutdown()
{
    WaitForIdle();

    g_vulkan.dynamic_upload_ring.Shutdown();
    g_vulkan.submission.Shutdown();

    g_vulkan.deferred_delete_queue.Flush();

    LOG_AND_SHUTDOWN_POOL(g_vulkan.buffer_pool);
    LOG_AND_SHUTDOWN_POOL(g_vulkan.texture_pool);
    LOG_AND_SHUTDOWN_POOL(g_vulkan.pipeline_state_pool);
    LOG_AND_SHUTDOWN_POOL(g_vulkan.shader_module_pool);
    LOG_AND_SHUTDOWN_POOL(g_vulkan.swapchain_pool);

    vkDestroyPipelineCache(g_vulkan.vk_device, g_vulkan.vk_pipeline_cache, nullptr);

    g_vulkan.descriptor_system.Shutdown(g_vulkan.vk_device);

    if (g_vulkan.vma_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(g_vulkan.vma_allocator);
        g_vulkan.vma_allocator = VK_NULL_HANDLE;
    }

    if (g_vulkan.vk_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(g_vulkan.vk_device, nullptr);
        g_vulkan.vk_device = VK_NULL_HANDLE;
    }

    if (g_vulkan.vk_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(g_vulkan.vk_instance, g_vulkan.vk_surface, nullptr);
        g_vulkan.vk_surface = VK_NULL_HANDLE;
    }

    // vkb_instance's destructor will handle VkInstance and debug messenger
    // No explicit call to vkb::destroy_instance(vkb_instance) needed if vkb_instance is a member
    // The VkInstance and VkDebugUtilsMessengerEXT handles within VkContext are associated with vkb_instance.
    // So, once vkb_instance is destroyed (implicitly when VkContext goes out of scope or explicitly if VkContext is static and its members need to be reset),
    // these handles are also handled. Setting them to VK_NULL_HANDLE here reflects that they are no longer valid.
    g_vulkan.vk_instance = VK_NULL_HANDLE;
    g_vulkan.vk_debug_messenger = VK_NULL_HANDLE;

    PHX_RHI_INFO("Vulkan Device Shutdown Complete.");
}

ShaderFormat phx::rhi::GetShaderFormat() { return ShaderFormat::Spirv; }
GfxBackend phx::rhi::GetBackend() { return GfxBackend::Vulkan; }

namespace phx::rhi::vulkan
{
    bool SelectPhysicalDevice(vkb::PhysicalDevice& out_vkb_physical_device)
    {
        vkb::PhysicalDeviceSelector selector{ g_vulkan.vkb_instance };
        VkPhysicalDeviceFeatures features_to_enable = {
            .depthClamp = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
            .shaderInt64 = VK_TRUE,
        };

        const std::vector<const char*> required_extensions =
        {
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
            VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
            VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_MULTIVIEW_EXTENSION_NAME,
            VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
        };

        selector.set_minimum_version(1, 3)
            .set_surface(g_vulkan.vk_surface)
            .set_required_features(features_to_enable)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .add_required_extensions(required_extensions.size(), required_extensions.data());

        VkPhysicalDeviceFeatures vulkan_features_1_0 = {};
        vulkan_features_1_0.samplerAnisotropy = VK_TRUE;
        vulkan_features_1_0.multiDrawIndirect = VK_TRUE; // Almost guaranteed you'll need this later

        // --- 1.1 Features (THE FIX) ---
        VkPhysicalDeviceVulkan11Features vulkan_features_1_1 = {};
        vulkan_features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        vulkan_features_1_1.shaderDrawParameters = VK_TRUE;

        VkPhysicalDeviceVulkan12Features vulkan_features_1_2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = nullptr,
            .shaderInt8 = VK_TRUE,
            .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        #if !USE_BUFFER_ADDRESS
            .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
            .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
        #endif
            .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
        #if !USE_BUFFER_ADDRESS
            .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
            .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
        #endif
            .descriptorBindingPartiallyBound = VK_TRUE,
            .runtimeDescriptorArray = VK_TRUE,
            .samplerFilterMinmax = VK_TRUE,
            .timelineSemaphore = VK_TRUE,
            .bufferDeviceAddress = VK_TRUE,
        };


        VkPhysicalDeviceVulkan13Features vulkan_features_1_3 = {};
        vulkan_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        vulkan_features_1_3.pNext = nullptr;

        vulkan_features_1_3.dynamicRendering = VK_TRUE;
        vulkan_features_1_3.synchronization2 = VK_TRUE;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state = {};
        extended_dynamic_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
        extended_dynamic_state.extendedDynamicState = VK_TRUE;

        VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
            .pNext = nullptr,
            .descriptorBuffer = VK_TRUE,
            .descriptorBufferCaptureReplay = VK_FALSE,
            .descriptorBufferPushDescriptors = VK_TRUE 
        };

        VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutable_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT,
            .mutableDescriptorType = VK_TRUE,
        };

        selector.set_required_features(vulkan_features_1_0);
        selector.set_required_features_11(vulkan_features_1_1);
        selector.set_required_features_12(vulkan_features_1_2);
        selector.set_required_features_13(vulkan_features_1_3);
        selector.add_required_extension_features(descriptor_buffer_features);
        selector.add_required_extension_features(extended_dynamic_state);
        selector.add_required_extension_features(mutable_features);

        // Add specific extension requirements if vkb doesn't infer them well enough
        auto phys_dev_ret = selector.select();
        if (!phys_dev_ret)
        {
            PHX_RHI_ERROR("[RHI] Failed to select suitable Physical Device: {0}", phys_dev_ret.error().message());
            return false;
        }

        const std::vector<const char*> optional_extensions =
        {
            VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,
            // VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
            VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
            VK_EXT_MESH_SHADER_EXTENSION_NAME,
        };
        out_vkb_physical_device.enable_extensions_if_present(optional_extensions);
        out_vkb_physical_device = phys_dev_ret.value();

        g_vulkan.vk_chosen_physical_device = out_vkb_physical_device.physical_device;
        g_vulkan.vk_descriptor_buffer_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
        g_vulkan.vk_physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        g_vulkan.vk_physical_device_properties.pNext = &g_vulkan.vk_descriptor_buffer_properties; // Chain the struct here

        vkGetPhysicalDeviceProperties2(g_vulkan.vk_chosen_physical_device, &g_vulkan.vk_physical_device_properties);

        g_vulkan.vk_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        g_vulkan.vk_features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        g_vulkan.vk_features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        g_vulkan.vk_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        g_vulkan.vk_features2.pNext = &g_vulkan.vk_features_1_1;
        g_vulkan.vk_features_1_1.pNext = &g_vulkan.vk_features_1_2;
        g_vulkan.vk_features_1_2.pNext = &g_vulkan.vk_features_1_3;

        vkGetPhysicalDeviceFeatures2(g_vulkan.vk_chosen_physical_device, &g_vulkan.vk_features2);

        PHX_CORE_ASSERT(g_vulkan.vk_features_1_2.bufferDeviceAddress == VK_TRUE);

        {
            uint32_t vk_api_major = VK_VERSION_MAJOR(g_vulkan.vk_physical_device_properties.properties.apiVersion);
            uint32_t vk_api_minor = VK_VERSION_MINOR(g_vulkan.vk_physical_device_properties.properties.apiVersion);
            uint32_t vk_api_patch = VK_VERSION_PATCH(g_vulkan.vk_physical_device_properties.properties.apiVersion);

            uint32_t driver_major = VK_VERSION_MAJOR(g_vulkan.vk_physical_device_properties.properties.driverVersion);
            uint32_t driver_minor = VK_VERSION_MINOR(g_vulkan.vk_physical_device_properties.properties.driverVersion);
            uint32_t driver_patch = VK_VERSION_PATCH(g_vulkan.vk_physical_device_properties.properties.driverVersion);
            PHX_RHI_INFO("Selected {0} with driver {1}.{2}.{3} with Vulkan API version: {4}.{5}.{6}",
                g_vulkan.vk_physical_device_properties.properties.deviceName,
                g_vulkan.vk_physical_device_properties.properties.driverVersion,
                driver_major,
                driver_minor,
                driver_patch,
                vk_api_major,
                vk_api_minor,
                vk_api_patch);
        }

        return true;
    }

    bool InitVma()
    {
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.physicalDevice = g_vulkan.vk_chosen_physical_device;
        allocator_info.device = g_vulkan.vk_device;
        allocator_info.instance = g_vulkan.vk_instance;
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

        allocator_info.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
            VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

        // VkPhysicalDeviceFeatures enabled_features; // Not needed, using vulkan_backend->vk_features_1_2 directly
        // vkGetPhysicalDeviceFeatures(vulkan_backend->vk_chosen_physical_device, &enabled_features); // Example, better to use vkb info

        if (g_vulkan.vk_features_1_2.bufferDeviceAddress)
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        VkResult res = vmaCreateAllocator(&allocator_info, &g_vulkan.vma_allocator);
        if (res != VK_SUCCESS)
        {
            PHX_RHI_ERROR("Failed to create VMA Allocator. VkResult: <TODO>");
            return false;
        }

        const VkPhysicalDeviceMemoryProperties* memory_properties;
        vmaGetMemoryProperties(g_vulkan.vma_allocator, &memory_properties);

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
                            g_vulkan.vk_rebar_heap_size = heap.size;
                            PHX_RHI_INFO("Rebar Heap found {0}", PhxToMB(g_vulkan.vk_rebar_heap_size));
                            break;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool CreateLogicalDevice(vkb::PhysicalDevice& vkb_physical_device)
    {
        vkb::DeviceBuilder device_builder{ vkb_physical_device };

        auto dev_ret = device_builder.build();
        if (!dev_ret)
        {
            PHX_RHI_ERROR("[RHI] Failed to create Logical Device: {0}", dev_ret.error().message());
            return false;
        }

        vkb::Device vkb_device = dev_ret.value();
        g_vulkan.vk_device = vkb_device.device;
        volkLoadDevice(g_vulkan.vk_device); // Load device-level functions

        // -- hitting a device_dispatch nullptr exception. this check is to see what is going on ---
        PHX_ASSERT(vkCmdPipelineBarrier2 != nullptr, "vkCmdPipelineBarrier2 function pointer is NULL! Check device feature enablement.");

        auto gfx_q_ret = vkb_device.get_queue(vkb::QueueType::graphics);
        if (!gfx_q_ret)
        {
            PHX_RHI_ERROR("[RHI] Failed to get graphics queue: {0}", gfx_q_ret.error().message());
            return false;
        }

        VulkanQueue& queue_gfx = g_vulkan.queues[CommandQueueType::Graphics];
        queue_gfx.vk_queue = gfx_q_ret.value();
        queue_gfx.vk_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
        PHX_RHI_INFO("[RHI Vulkan] Using graphics queue {0}", queue_gfx.vk_queue_family);


        VulkanQueue& queue_compute = g_vulkan.queues[CommandQueueType::Compute];
        VulkanQueue& queue_transfer = g_vulkan.queues[CommandQueueType::Copy];

        auto compute_q_ret = vkb_device.get_queue(vkb::QueueType::compute);
        if (!compute_q_ret)
        {
            PHX_RHI_WARN("[RHI] Failed to get dedicated compute queue, using graphics queue: {0}", compute_q_ret.error().message());

            queue_compute.vk_queue = queue_gfx.vk_queue;
            queue_compute.vk_queue_family = queue_gfx.vk_queue_family;
        }
        else
        {
            queue_compute.vk_queue = compute_q_ret.value();
            queue_compute.vk_queue_family = vkb_device.get_queue_index(vkb::QueueType::compute).value();

            PHX_RHI_INFO("[RHI Vulkan] Using compute queue {0}", queue_compute.vk_queue_family);
        }

        auto transfer_q_ret = vkb_device.get_queue(vkb::QueueType::transfer);
        if (!transfer_q_ret)
        {
            PHX_RHI_WARN("[RHI] Failed to get dedicated transfer queue, using graphics queue: {0}", transfer_q_ret.error().message());
            queue_transfer.vk_queue = queue_gfx.vk_queue;
            queue_transfer.vk_queue_family = queue_gfx.vk_queue_family;
        }
        else
        {
            queue_transfer.vk_queue = transfer_q_ret.value();
            queue_transfer.vk_queue_family = vkb_device.get_queue_index(vkb::QueueType::transfer).value();

            PHX_RHI_INFO("[RHI Vulkan] Using transfer queue {0}", queue_transfer.vk_queue_family);
        }

        return true;
    }


    bool CreateSurface(void* window_handle)
    {
        if (!window_handle)
        {
            PHX_CORE_ERROR("[RHI] Window Handle is null in RhiDescriptor.");
            return false;
        }

#if defined(PHX_PLATFORM_WINDOWS)
        VkWin32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext = nullptr;
        surface_create_info.flags = 0;
        surface_create_info.hwnd = static_cast<HWND>(window_handle);
        surface_create_info.hinstance = GetModuleHandle(NULL);

        VkResult result = vkCreateWin32SurfaceKHR(g_vulkan.vk_instance, &surface_create_info, nullptr, &g_vulkan.vk_surface);
        if (result != VK_SUCCESS)
        {
            PHX_CORE_ERROR("[RHI] Failed to create Win32 surface. VkResult: <TODO>");
            return false;
        }

#elif defined(PHX_PLATFORM_LINUX)

    WaylandHandles* wayland_handles = reinterpret_cast<WaylandHandles*>(window_handle);

    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = wayland_handles->display;
    createInfo.surface = wayland_handles->surface;

    VkResult result = vkCreateWaylandSurfaceKHR(g_vulkan.vk_instance, &createInfo, nullptr, &g_vulkan.vk_surface);
    if (result != VK_SUCCESS) 
    {
        PHX_CORE_ERROR("[RHI] Failed to create Wayland surface. VkResult: <TODO>");
        return false;
    }
#else
#error "Vulkan surface creation not implemented for this platform."
#endif
        return true;
    }
}

inline static VKAPI_ATTR VkBool32 VKAPI_CALL vk_phx_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*)
{
#if false
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        PHX_RHI_TRACE("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
#else
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
#endif
    {
        PHX_RHI_INFO("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        PHX_RHI_WARN("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        PHX_RHI_ERROR("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    return VK_FALSE;
}