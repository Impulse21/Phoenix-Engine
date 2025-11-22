#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanBackend.h"


#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-private-field"

#endif


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"


#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "PhxCore/Math.h"

#ifdef PHX_PLATFORM_WINDOWS
extern HINSTANCE g_hInstance;
#endif

#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_RHI_WARN("[Vulkan] - Pool '" #x "' still contains active handles"); x.Shutdown();

inline static VKAPI_ATTR VkBool32 VKAPI_CALL vk_phx_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void*);


bool phx::rhi::VulkanBackend::Initialize()
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

    vkb_instance = inst_ret.value();
    vk_instance = vkb_instance.instance;
    vk_debug_messenger = vkb_instance.debug_messenger;
    volkLoadInstance(vk_instance);

#if defined(PHX_PLATFORM_WINDOWS)
    if (!CreateSurface_Win32(static_cast<HWND>(window_handle)))
    {
        vkb::destroy_instance(vkb_instance);
        return false;
    }
#else
	PHX_RHI_ERROR("Vulkan surface creation not implemented for this platform.");
    vkb::destroy_instance(VkContext::vkb_instance);
    return false;
#endif

    vkb::PhysicalDevice vkb_physical_device; // Renamed to snake_case
    if (!SelectPhysicalDevice(vkb_physical_device))
    {
        vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
        vkb::destroy_instance(vkb_instance);
        return false;
    }

    if (!CreateLogicalDevice(vkb_physical_device))
    {
        vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
        vkb::destroy_instance(vkb_instance);
        return false;
    }

	return false;
}


void phx::rhi::VulkanBackend::Shutdown()
{
    if (vk_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(vk_device, nullptr);
        vk_device = VK_NULL_HANDLE;
    }

    if (vk_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
        vk_surface = VK_NULL_HANDLE;
    }

    // vkb_instance's destructor will handle VkInstance and debug messenger
    // No explicit call to vkb::destroy_instance(vkb_instance) needed if vkb_instance is a member
    // The VkInstance and VkDebugUtilsMessengerEXT handles within VkContext are associated with vkb_instance.
    // So, once vkb_instance is destroyed (implicitly when VkContext goes out of scope or explicitly if VkContext is static and its members need to be reset),
    // these handles are also handled. Setting them to VK_NULL_HANDLE here reflects that they are no longer valid.
    vk_instance = VK_NULL_HANDLE;
    vk_debug_messenger = VK_NULL_HANDLE;

    PHX_RHI_INFO("Vulkan Device Shutdown Complete.");
}

phx::rhi::VulkanBackend::VulkanBackend(void* window_handle)
    : window_handle(window_handle)
{
}

bool phx::rhi::VulkanBackend::SelectPhysicalDevice(vkb::PhysicalDevice& out_vkb_physical_device)
{
    vkb::PhysicalDeviceSelector selector{ vkb_instance };
    VkPhysicalDeviceFeatures features_to_enable = {};
    features_to_enable.samplerAnisotropy = VK_TRUE;
    // Add other features you absolutely need enabled

    const std::vector<const char*> required_extensions =
    {
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
    };

    selector.set_minimum_version(1, 3)
        .set_surface(vk_surface)
        .set_required_features(features_to_enable)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .add_required_extensions(required_extensions.size(), required_extensions.data());

    VkPhysicalDeviceVulkan12Features vulkan_features_1_2 = {};
    vulkan_features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan_features_1_2.pNext = nullptr;

    vulkan_features_1_2.bufferDeviceAddress = VK_TRUE;
    vulkan_features_1_2.runtimeDescriptorArray = VK_TRUE;
    vulkan_features_1_2.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan_features_1_2.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan_features_1_2.timelineSemaphore = VK_TRUE;
    vulkan_features_1_2.samplerFilterMinmax = VK_TRUE;


    selector.add_required_extension_features(vulkan_features_1_2);

    VkPhysicalDeviceVulkan13Features vulkan_features_1_3 = {};
    vulkan_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan_features_1_3.pNext = nullptr;

    vulkan_features_1_3.dynamicRendering = VK_TRUE;
    vulkan_features_1_3.synchronization2 = VK_TRUE;

    selector.add_required_extension_features(vulkan_features_1_3);

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

    vk_chosen_physical_device = out_vkb_physical_device.physical_device;

    vk_descriptor_buffer_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

    vk_physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vk_physical_device_properties.pNext = &vk_descriptor_buffer_properties; // Chain the struct here

    vkGetPhysicalDeviceProperties2(vk_chosen_physical_device, &vk_physical_device_properties);

    vk_features2.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vk_features_1_1.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vk_features_1_2.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vk_features_1_3.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    vk_features2.pNext      = &vk_features_1_1;
    vk_features_1_1.pNext   = &vk_features_1_2;
    vk_features_1_2.pNext   = &vk_features_1_3;

    vkGetPhysicalDeviceFeatures2(vk_chosen_physical_device, &vk_features2);

    PHX_CORE_ASSERT(vk_features_1_2.bufferDeviceAddress == VK_TRUE);

    {
        uint32_t major = VK_VERSION_MAJOR(vk_physical_device_properties.properties.apiVersion);
        uint32_t minor = VK_VERSION_MINOR(vk_physical_device_properties.properties.apiVersion);
        uint32_t patch = VK_VERSION_PATCH(vk_physical_device_properties.properties.apiVersion);

        PHX_RHI_INFO("Selected Devices API version: {0}.{1}.{2}",
            major,
            minor,
            patch);
    }

    return true;
}

bool phx::rhi::VulkanBackend::CreateLogicalDevice(vkb::PhysicalDevice& vkb_physical_device)
{
    vkb::DeviceBuilder device_builder{ vkb_physical_device };

    auto dev_ret = device_builder.build();
    if (!dev_ret)
    {
        PHX_RHI_ERROR("[RHI] Failed to create Logical Device: {0}", dev_ret.error().message());
        return false;
    }

    vkb::Device vkb_device = dev_ret.value();
    vk_device = vkb_device.device;
    volkLoadDevice(vk_device); // Load device-level functions

    // -- hitting a device_dispatch nullptr exception. this check is to see what is going on ---
    PHX_ASSERT(vkCmdPipelineBarrier2 != nullptr, "vkCmdPipelineBarrier2 function pointer is NULL! Check device feature enablement.");

    auto gfx_q_ret = vkb_device.get_queue(vkb::QueueType::graphics);
    if (!gfx_q_ret)
    {
        PHX_RHI_ERROR("[RHI] Failed to get graphics queue: {0}", gfx_q_ret.error().message());
        return false;
    }

    Queue& queue_gfx = queues[CommandQueueType::Graphics];
    queue_gfx.vk_queue = gfx_q_ret.value();
    queue_gfx.vk_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    PHX_RHI_INFO("[RHI Vulkan] Using graphics queue {0}", queue_gfx.vk_queue_family);


    Queue& queue_compute = queues[CommandQueueType::Compute];
    Queue& queue_transfer = queues[CommandQueueType::Copy];

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


#if defined(PHX_PLATFORM_WINDOWS)
bool phx::rhi::VulkanBackend::CreateSurface_Win32(void* window_handle)
{
    if (!window_handle)
    {
        PHX_CORE_ERROR("[RHI] WindowsHandle is null in RhiDescriptor.");
        return false;
    }

    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.flags = 0;
    surface_create_info.hwnd = static_cast<HWND>(window_handle);
    surface_create_info.hinstance = g_hInstance;

    VkResult result = vkCreateWin32SurfaceKHR(vk_instance, &surface_create_info, nullptr, &vk_surface);
    if (result != VK_SUCCESS)
    {
        PHX_CORE_ERROR("[RHI] Failed to create Win32 surface. VkResult: <TODO>");
        return false;
    }
    return true;
}

#endif

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
