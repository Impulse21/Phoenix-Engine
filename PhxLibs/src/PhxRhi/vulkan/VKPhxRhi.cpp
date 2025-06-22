#include "PhxRhi/PhxRhi_pch.h"

#include <PhxRhi/PhxRhi.h>

#include "VkRhi_Internal.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
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

#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_CORE_WARN("[Vulkan] - Pool '" #x "' still contains active handles"); x.Shutdown();

using namespace phx;
using namespace phx::RHI;

namespace
{
    VkAllocationCallbacks* GetVkAllocationCallbacks()
    {

#if USE_PHX_ALLOCATOR
        return &m_allocCallbacks;
#else
        return nullptr;
#endif
    }

#if USE_PHX_ALLOCATOR
    static void* VKAPI_CALL vk_phx_allocate(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope);
    static void* VKAPI_CALL vk_phx_reallocate(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope);
    static void VKAPI_CALL vk_phx_free(void* pUserData, void* pMemory);
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
            PHX_CORE_TRACE("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
#else
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
#endif
        {
            PHX_CORE_INFO("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            PHX_CORE_WARN("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            PHX_CORE_ERROR("[Vulkan Debug] {0}\n\t{1}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        }
        return VK_FALSE;
    }


    void InitializeVolk()
    {
        if (!VkContext::volk_initialized)
        {
            if (volkInitialize() != VK_SUCCESS)
            {
                PHX_CORE_ERROR("[RHI] Failed to initialize Volk.");
                return;
            }
            VkContext::volk_initialized = true;
        }
    }

    bool CreateInstance(const RhiDescriptor& desc) // Changed RhiDescriptor back to RhiDescriptor for consistency with later functions
    {
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

#if USE_PHX_ALLOCATOR
        // This assumes `alloc_callbacks` is a local variable, not a member of VkContext.
        // If it needs to persist, consider adding it as an inline static member to VkContext.
        VkAllocationCallbacks alloc_callbacks = {
            .pUserData = &phx::Memory::g_persistentAllocator,
            .pfnAllocation = vk_phx_allocate,
            .pfnReallocation = vk_phx_reallocate,
            .pfnFree = vk_phx_free,
            .pfnInternalAllocation = nullptr,
            .pfnInternalFree = nullptr
        };

        builder.set_allocation_callbacks(&alloc_callbacks);
#endif

        auto inst_ret = builder.build();
        if (!inst_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to create Vulkan Instance: {0}", inst_ret.error().message());
            return false;
        }

        VkContext::vkb_instance = inst_ret.value();
        VkContext::vk_instance = VkContext::vkb_instance.instance;
        VkContext::vk_debug_messenger = VkContext::vkb_instance.debug_messenger;
        volkLoadInstance(VkContext::vk_instance); // Load instance-level functions
        return true;
    }

    bool CreateSurface(const RhiDescriptor& desc)
    {
#ifdef PHX_PLATFORM_WINDOWS
        if (!desc.WindowsHandle)
        {
            PHX_CORE_ERROR("[RHI] WindowsHandle is null in RhiDescriptor.");
            return false;
        }

        VkWin32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext = nullptr;
        surface_create_info.flags = 0;
        surface_create_info.hwnd = static_cast<HWND>(desc.WindowsHandle);
        surface_create_info.hinstance = g_hInstance;

        VkResult result = vkCreateWin32SurfaceKHR(VkContext::vk_instance, &surface_create_info, GetVkAllocationCallbacks(), &VkContext::vk_surface);
        if (result != VK_SUCCESS)
        {
            PHX_CORE_ERROR("[RHI] Failed to create Win32 surface. VkResult: <TODO>");
            return false;
        }
        return true;
#else
        PHX_CORE_ERROR("[RHI] Platform not supported for surface creation yet.");
        return false;
#endif
    }

    bool SelectPhysicalDevice(const RhiDescriptor&, vkb::PhysicalDevice& out_vkb_physical_device)
    {
        vkb::PhysicalDeviceSelector selector{ VkContext::vkb_instance };
        VkPhysicalDeviceFeatures features_to_enable = {};
        features_to_enable.samplerAnisotropy = VK_TRUE;
        // Add other features you absolutely need enabled

        const std::vector<const char*> required_extensions =
        {
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
            VK_KHR_MULTIVIEW_EXTENSION_NAME,
            VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
        };

        selector.set_minimum_version(1, 3)
            .set_surface(VkContext::vk_surface)
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
            PHX_CORE_ERROR("[RHI] Failed to select suitable Physical Device: {0}", phys_dev_ret.error().message());
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

        VkContext::vk_chosen_physical_device = out_vkb_physical_device.physical_device;

        VkContext::vk_descriptor_buffer_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

        VkContext::vk_physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        VkContext::vk_physical_device_properties.pNext = &VkContext::vk_descriptor_buffer_properties; // Chain the struct here

        vkGetPhysicalDeviceProperties2(VkContext::vk_chosen_physical_device, &VkContext::vk_physical_device_properties);

        VkContext::vk_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        VkContext::vk_features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        VkContext::vk_features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        VkContext::vk_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        VkContext::vk_features2.pNext = &VkContext::vk_features_1_1;
        VkContext::vk_features_1_1.pNext = &VkContext::vk_features_1_2;
        VkContext::vk_features_1_2.pNext = &VkContext::vk_features_1_3;

        vkGetPhysicalDeviceFeatures2(VkContext::vk_chosen_physical_device, &VkContext::vk_features2);

        PHX_CORE_ASSERT(VkContext::vk_features_1_2.bufferDeviceAddress == VK_TRUE);

        return true;
    }

    bool CreateLogicalDevice(const RhiDescriptor&, vkb::PhysicalDevice& vkb_physical_device)
    {
        vkb::DeviceBuilder device_builder{ vkb_physical_device };

        auto dev_ret = device_builder.build();
        if (!dev_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to create Logical Device: {0}", dev_ret.error().message());
            return false;
        }

        vkb::Device vkb_device = dev_ret.value();
        VkContext::vk_device = vkb_device.device;
        volkLoadDevice(VkContext::vk_device); // Load device-level functions

        auto gfx_q_ret = vkb_device.get_queue(vkb::QueueType::graphics);
        if (!gfx_q_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to get graphics queue: {0}", gfx_q_ret.error().message());
            return false;
        }

        VkContext::vk_graphics_queue = gfx_q_ret.value();
        VkContext::vk_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
        PHX_CORE_INFO("[RHI Vulkan] Using graphics queue {0}", VkContext::vk_graphics_queue_family);

        auto compute_q_ret = vkb_device.get_queue(vkb::QueueType::compute);
        if (!compute_q_ret)
        {
            PHX_CORE_WARN("[RHI] Failed to get dedicated compute queue, using graphics queue: {0}", compute_q_ret.error().message());
            VkContext::vk_compute_queue = VkContext::vk_graphics_queue;
            VkContext::vk_compute_queue_family = VkContext::vk_graphics_queue_family;
        }
        else
        {
            VkContext::vk_compute_queue = compute_q_ret.value();
            VkContext::vk_compute_queue_family = vkb_device.get_queue_index(vkb::QueueType::compute).value();

            PHX_CORE_INFO("[RHI Vulkan] Using compute queue {0}", VkContext::vk_compute_queue_family);
        }

        auto transfer_q_ret = vkb_device.get_queue(vkb::QueueType::transfer);
        if (!transfer_q_ret)
        {
            PHX_CORE_WARN("[RHI] Failed to get dedicated transfer queue, using graphics queue: {0}", transfer_q_ret.error().message());
            VkContext::vk_transfer_queue = VkContext::vk_graphics_queue;
            VkContext::vk_transfer_queue_family = VkContext::vk_graphics_queue_family;
        }
        else
        {
            VkContext::vk_transfer_queue = transfer_q_ret.value();
            VkContext::vk_transfer_queue_family = vkb_device.get_queue_index(vkb::QueueType::transfer).value();

            PHX_CORE_INFO("[RHI Vulkan] Using transfer queue {0}", VkContext::vk_transfer_queue_family);
        }

        return true;
    }

    bool CreateAllocator(const RhiDescriptor&)
    {
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.physicalDevice = VkContext::vk_chosen_physical_device;
        allocator_info.device = VkContext::vk_device;
        allocator_info.instance = VkContext::vk_instance;
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

        allocator_info.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
            VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

        // VkPhysicalDeviceFeatures enabled_features; // Not needed, using VkContext::vk_features_1_2 directly
        // vkGetPhysicalDeviceFeatures(VkContext::vk_chosen_physical_device, &enabled_features); // Example, better to use vkb info

        if (VkContext::vk_features_1_2.bufferDeviceAddress)
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        VkResult res = vmaCreateAllocator(&allocator_info, &VkContext::vma_allocator);
        if (res != VK_SUCCESS)
        {
            PHX_CORE_ERROR("[RHI] Failed to create VMA Allocator. VkResult: <TODO>");
            return false;
        }

        const VkPhysicalDeviceMemoryProperties* memory_properties;
        vmaGetMemoryProperties(VkContext::vma_allocator, &memory_properties);

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
                            VkContext::vk_rebar_heap_size = heap.size;
                            PHX_CORE_INFO("Rebar Heap found {0}", PhxToMB(VkContext::vk_rebar_heap_size));
                            break;
                        }
                    }
                }
            }
        }
        return true;
    }

    void CreateSwapchain(const RhiDescriptor& desc)
    {
        PHX_PROFILE_SECTION("Vulkan::CreateSwapchain");
        vkb::SwapchainBuilder swapchain_builder(VkContext::vk_chosen_physical_device, VkContext::vk_device, VkContext::vk_surface); // Renamed to snake_case

        auto swap_ret = swapchain_builder
            .set_desired_format({ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(desc.SwapChainDesc.Width, desc.SwapChainDesc.Height)
            .set_old_swapchain(VK_NULL_HANDLE) // For initial creation
            .build();

        if (!swap_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to create swapchain: {0}", swap_ret.error().message());
            // This is a critical failure during init
            return;
        }

        vkb::Swapchain vkb_swapchain = swap_ret.value(); // Renamed to snake_case
        VkContext::vk_swapchain = vkb_swapchain.swapchain;
        VkContext::vk_swapchain_image_format = vkb_swapchain.image_format;
        VkContext::vk_swapchain_extent = vkb_swapchain.extent;

        VkContext::vk_swapchain_images = vkb_swapchain.get_images().value();
        VkContext::vk_swapchain_image_views = vkb_swapchain.get_image_views().value();

        PHX_CORE_INFO(
            "[RHI] Swapchain Initialized. Extent: {0}x{1}, Format: {2}, Images: {3}",
            VkContext::vk_swapchain_extent.width,
            VkContext::vk_swapchain_extent.height,
            "", // Placeholder for format name conversion
            VkContext::vk_swapchain_images.size());
    }

    void CleanupSwapchain()
    {
        if (VkContext::vk_device == VK_NULL_HANDLE)
            return;

        for (auto image_view : VkContext::vk_swapchain_image_views) // Renamed to snake_case
        {
            if (image_view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(VkContext::vk_device, image_view, GetVkAllocationCallbacks());
            }
        }
        VkContext::vk_swapchain_image_views.clear();
        VkContext::vk_swapchain_images.clear();

        if (VkContext::vk_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(VkContext::vk_device, VkContext::vk_swapchain, GetVkAllocationCallbacks());
            VkContext::vk_swapchain = VK_NULL_HANDLE;
        }
    }

    void RecreateSwapchain(const RhiDescriptor& desc)
    {
        WaitForIdle();
        CleanupSwapchain();
        CreateSwapchain(desc);
    }

    void CreateFrameSyncObjects()
    {
        PHX_PROFILE_SECTION("Vulkan::CreateFrameSyncObjects");
        VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; // Renamed to snake_case
        VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; // Renamed to snake_case
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < cMaxInflightFrames; ++i)
        {
            VkResult result = vkCreateSemaphore(VkContext::vk_device, &semaphore_info, GetVkAllocationCallbacks(), &VkContext::frames[i].PresentSemaphore);
            PHX_CORE_ASSERT(result == VK_SUCCESS);

            result = vkCreateSemaphore(VkContext::vk_device, &semaphore_info, GetVkAllocationCallbacks(), &VkContext::frames[i].RenderSemaphore);
            PHX_CORE_ASSERT(result == VK_SUCCESS);

            result = vkCreateFence(VkContext::vk_device, &fence_info, GetVkAllocationCallbacks(), &VkContext::frames[i].RenderFence);
            PHX_CORE_ASSERT(result == VK_SUCCESS);
        }

        PHX_CORE_INFO("[RHI] Frame synchronization primitives created.");
    }

    void DestroyFrameSyncObjects()
    {
        if (VkContext::vk_device == VK_NULL_HANDLE) return;
        for (size_t i = 0; i < cMaxInflightFrames; ++i)
        {
            if (VkContext::frames[i].RenderFence != VK_NULL_HANDLE)
            {
                vkDestroyFence(VkContext::vk_device, VkContext::frames[i].RenderFence, GetVkAllocationCallbacks());
                VkContext::frames[i].RenderFence = VK_NULL_HANDLE;
            }
            if (VkContext::frames[i].RenderSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(VkContext::vk_device, VkContext::frames[i].RenderSemaphore, GetVkAllocationCallbacks());
                VkContext::frames[i].RenderSemaphore = VK_NULL_HANDLE;
            }
            if (VkContext::frames[i].PresentSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(VkContext::vk_device, VkContext::frames[i].PresentSemaphore, GetVkAllocationCallbacks());
                VkContext::frames[i].PresentSemaphore = VK_NULL_HANDLE;
            }
        }
        PHX_CORE_INFO("[RHI] Frame synchronization primitives destroyed.");
    }

    void CreateCommandPools()
    {
        PHX_PROFILE_SECTION("Vulkan::CreateCommandPools");

        VkCommandPoolCreateInfo pool_info = {}; // Renamed to snake_case
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = VkContext::vk_graphics_queue_family;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VkResult result = vkCreateCommandPool(VkContext::vk_device, &pool_info, GetVkAllocationCallbacks(), &VkContext::vk_graphics_command_pool);

        PHX_CORE_ASSERT(result == VK_SUCCESS);
        PHX_CORE_INFO("[RHI] Graphics Command Pool created.");
        // Create other command pools (compute, transfer) if needed
    }

    void DestroyCommandPools()
    {
        if (VkContext::vk_graphics_command_pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(VkContext::vk_device, VkContext::vk_graphics_command_pool, GetVkAllocationCallbacks());
            VkContext::vk_graphics_command_pool = VK_NULL_HANDLE;
            PHX_CORE_INFO("[RHI] Graphics Command Pool destroyed.");
        }
    }

    void InitializeResourcePools()
    {
        // TODO: Data drive these
        VkContext::buffer_pool.Initialize(kMaxNumBuffers);
    }

    void ShutdownResourcePools()
    {
        LOG_AND_SHUTDOWN_POOL(VkContext::buffer_pool);
    }

    int CreateSubResource(Buffer_VK& buffer, GpuBufferDescriptor const& desc, SubresouceType subresource_type, size_t offset, size_t size = ~0u)
    {
        assert(subresource_type == SubresouceType::SRV || subresource_type == SubresouceType::UAV);

        Format format = desc.Format;

        // Is raw buffer
        if (format == Format::UNKNOWN)
        {
            buffer.srv_is_typed = false;
#if false
            // These sections are commented out in the original code, keeping them commented.
            // If uncommented, they would need VkContext:: prefix for m_bindlessStorageBuffers and m_device
            // buffer.srv_index = VkContext::bindless_storage_buffers.Allocate(); // Assuming a new name for this member

            // VkDescriptorBufferInfo buffer_info = {}; // Renamed to snake_case
            // buffer_info.buffer = buffer.vk_buffer;
            // buffer_info.offset = offset;
            // buffer_info.range = size;

            // VkWriteDescriptorSet write = {};
            // write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            // write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            // write.dstBinding = 0;
            // write.dstArrayElement = buffer.srv_index;
            // write.descriptorCount = 1;
            // write.dstSet = VkContext::bindless_storage_buffers.DescritporSetVk; // Assuming new name
            // write.pBufferInfo = &buffer_info;

            // vkUpdateDescriptorSets(VkContext::vk_device, 1, &write, 0, nullptr);
#else
            PHX_CORE_WARN("[Vulkan] TODO: Add Bindless support");
#endif
        }
        else
        {
            // Typed buffer
            buffer.srv_is_typed = true;

            VkBufferViewCreateInfo srv_desc = {}; // Renamed to snake_case
            srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            srv_desc.buffer = buffer.vk_buffer;
            srv_desc.flags = 0;
            srv_desc.format = FormatToVkFormat(format);
            srv_desc.offset = offset;
            srv_desc.range = std::min(size, (uint64_t)desc.Size - srv_desc.offset);

            VkResult res = vkCreateBufferView(VkContext::vk_device, &srv_desc, nullptr, &buffer.buffer_view);
            assert(res == VK_SUCCESS);

            if (subresource_type == SubresouceType::SRV)
            {
                // buffer.srv_index = VkContext::bindless_uniform_texel_buffers.Allocate(); // Assuming new name
                if (buffer.buffer_view != VK_NULL_HANDLE)
                {
                    VkWriteDescriptorSet write = {};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                    write.dstBinding = 0;
                    write.dstArrayElement = buffer.srv_index;
                    write.descriptorCount = 1;
                    // write.dstSet = VkContext::bindless_uniform_texel_buffers.DescritporSetVk; // Assuming new name
                    write.pTexelBufferView = &buffer.buffer_view;
                    vkUpdateDescriptorSets(VkContext::vk_device, 1, &write, 0, nullptr);
                }

                return -1;
            }
            else
            {
                // buffer.uav_index = VkContext::bindless_storage_texel_buffers.Allocate(); // Assuming new name
                if (buffer.buffer_view != VK_NULL_HANDLE)
                {
                    VkWriteDescriptorSet write = {};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                    write.dstBinding = 0;
                    write.dstArrayElement = buffer.uav_index;
                    write.descriptorCount = 1;
                    // write.dstSet = VkContext::bindless_storage_texel_buffers.DescritporSetVk; // Assuming new name
                    write.pTexelBufferView = &buffer.buffer_view;
                    vkUpdateDescriptorSets(VkContext::vk_device, 1, &write, 0, nullptr);
                }
                return -1;
            }
        }

        return 0;
    }

    void InitializeDescriptorBuffers()
    {
        VkContext::texture_descriptors.Initialize(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kMaxNumTextures); // Assuming 'this' can be replaced by 'nullptr' or another global context if needed
    }

    void ShutdownDescriptorBuffers()
    {
        VkContext::texture_descriptors.Shutdown();
    }
}

namespace phx::RHI
{
	bool Initialize(RhiDescriptor const& desc)
	{
		PHX_PROFILE_SECTION("Vulkan::PlatformInitialize");
		PHX_CORE_INFO("Initializing RHI (Vulkan) - VkGfxDeviceImpl");

		InitializeVolk();

		VkAllocationCallbacks* allocator_callbacks = GetVkAllocationCallbacks(); // Renamed to snake_case
		if (!CreateInstance(desc))
			return false;

		if (!CreateSurface(desc))
		{
			vkb::destroy_instance(VkContext::vkb_instance);
			return false;
		}

		vkb::PhysicalDevice vkb_physical_device; // Renamed to snake_case
		if (!SelectPhysicalDevice(desc, vkb_physical_device))
		{
			vkDestroySurfaceKHR(VkContext::vk_instance, VkContext::vk_surface, allocator_callbacks);
			vkb::destroy_instance(VkContext::vkb_instance);
			return false;
		}

		if (!CreateLogicalDevice(desc, vkb_physical_device))
		{
			vkDestroySurfaceKHR(VkContext::vk_instance, VkContext::vk_surface, allocator_callbacks);
			vkb::destroy_instance(VkContext::vkb_instance);
			return false;
		}

		if (!CreateAllocator(desc))
		{
			vkDestroyDevice(VkContext::vk_device, allocator_callbacks);
			vkDestroySurfaceKHR(VkContext::vk_instance, VkContext::vk_surface, allocator_callbacks);
			vkb::destroy_instance(VkContext::vkb_instance);
			return false;
		}

		CreateCommandPools();
		InitializeResourcePools();
		InitializeDescriptorBuffers();

		VkDeviceSize temp_allocator_size = 256_MiB; // Random Default
		if (VkContext::vk_rebar_heap_size > 0)
			temp_allocator_size = VkContext::vk_rebar_heap_size;

		VkContext::temp_memory_allocator.Initialize(math::GetPreviousPowerOfTwo(temp_allocator_size), 4_MiB);

		CreateSwapchain(desc); // Initial swapchain creation
		CreateFrameSyncObjects();

		VkContext::vk_swapchain_extent = { desc.SwapChainDesc.Width, desc.SwapChainDesc.Height }; // Updated to use the context variable

		VkContext::copy_ctx_manager.Initialize();

		VkContext::is_initialized = true; // Updated to use the context variable

		PHX_CORE_INFO("[RHI] Vulkan Initialized Successfully.");
		return true;
	}

	void Shutdown()
	{
        if (!VkContext::is_initialized)
        {
            return;
        }

        PHX_PROFILE_SECTION("Vulkan::PlatformShutdown");
        PHX_CORE_INFO("Shutting down RHI (Vulkan) - VkGfxDeviceImpl");

        WaitForIdle(); // Assuming this is a global function

        // These need to be shutdown before processing the delete queue
        // to ensure their resources are cleaned up.
        VkContext::copy_ctx_manager.Shutdown();
        VkContext::temp_memory_allocator.Shutdown();
        ShutdownDescriptorBuffers(); // This function was already updated in the previous turn

        VkContext::ProcessDeletionQueue(UINT64_MAX); // Assuming this is a global function

        DestroyFrameSyncObjects(); // This function was already updated
        CleanupSwapchain();       // This function was already updated

        ShutdownResourcePools(); // This function was already updated

        DestroyCommandPools(); // This function was already updated

        if (VkContext::vma_allocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(VkContext::vma_allocator);
            VkContext::vma_allocator = VK_NULL_HANDLE;
        }

        // vkb::Device doesn't have a destructor, must be explicit if not member
        // In the VkContext, vk_device is a VkDevice handle, not a vkb::Device object.
        // It's already handled by the vkb_instance or needs explicit destruction.
        if (VkContext::vk_device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(VkContext::vk_device, GetVkAllocationCallbacks());
            VkContext::vk_device = VK_NULL_HANDLE;
        }

        if (VkContext::vk_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(VkContext::vk_instance, VkContext::vk_surface, GetVkAllocationCallbacks());
            VkContext::vk_surface = VK_NULL_HANDLE;
        }

        // VkContext::vkb_instance's destructor will handle VkInstance and debug messenger
        // No explicit call to vkb::destroy_instance(VkContext::vkb_instance) needed if vkb_instance is a member
        // The VkInstance and VkDebugUtilsMessengerEXT handles within VkContext are associated with vkb_instance.
        // So, once vkb_instance is destroyed (implicitly when VkContext goes out of scope or explicitly if VkContext is static and its members need to be reset),
        // these handles are also handled. Setting them to VK_NULL_HANDLE here reflects that they are no longer valid.
        VkContext::vk_instance = VK_NULL_HANDLE;
        VkContext::vk_debug_messenger = VK_NULL_HANDLE;


        VkContext::is_initialized = false;
        PHX_CORE_INFO("[RHI] Vulkan Device Shutdown Complete.");
	}

	CommandBufferHandle BeginFrameCommandBuffer(CommandQueueType type)
	{
        // Request a command Queue
        return {};
	}

	CommandBufferHandle BeginAsyncCommandBuffer(CommandQueueType type)
	{
        return {};
	}

	void SubmitAsyncCommandBuffer(phx::Span<CommandBufferHandle> /*contexts*/)
	{
	}

	void SubmitAndPresentFrame()
	{

	}

	void WaitForIdle()
	{
        PHX_CORE_ASSERT(VkContext::is_initialized && VkContext::vk_device != VK_NULL_HANDLE);
        vkDeviceWaitIdle(VkContext::vk_device);
	}

	GpuBufferHandle CreateBuffer(const GpuBufferDescriptor& desc, const void* initial_data)
	{
        PHX_PROFILE_SECTION("Vulkan::PlatformCreateBuffer");
        if (VkContext::vma_allocator == VK_NULL_HANDLE)
            return GpuBufferHandle();

        Handle<GpuBuffer> ret_val = VkContext::buffer_pool.Allocate(); // Renamed to snake_case
        Buffer_VK& impl = *VkContext::buffer_pool.GetHot(ret_val); // Corrected access to buffer_pool

        VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; // Renamed to snake_case
        buffer_info.size = desc.Size;
        buffer_info.usage = 0;

        static const std::vector <std::pair<BindingFlags, VkBufferUsageFlags>> k_usage_mapping = // Renamed to snake_case
        {
            { BindingFlags::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
            { BindingFlags::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
            { BindingFlags::ConstantBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
            { BindingFlags::ShaderResource, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT},
            { BindingFlags::UnorderedAccess, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT},
        };

        for (const auto& [flag, usage_flag] : k_usage_mapping) // Renamed loop variables
        {
            if (phx::EnumHasAnyFlags(desc.BindingFlags, flag))
            {
                buffer_info.usage |= usage_flag;
            }
        }

        // Misc Flags
        static const std::vector <std::pair<ResourceMiscFlags, VkBufferUsageFlags>> k_usage_mapping_misc = // Renamed to snake_case
        {
            { ResourceMiscFlags::BufferRaw, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
            { ResourceMiscFlags::BufferStructured, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
            { ResourceMiscFlags::IndirectArgs, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},
            { ResourceMiscFlags::RayTracing, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR },
            { ResourceMiscFlags::DescriptorTable, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT },
        };

        for (const auto& [flag, usage_flag] : k_usage_mapping_misc) // Renamed loop variables
        {
            if (EnumHasAnyFlags(desc.MiscFlags, flag))
            {
                buffer_info.usage |= usage_flag;
            }
        }

        if (VkContext::vk_features_1_2.bufferDeviceAddress == VK_TRUE)
        {
            buffer_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }

        buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        buffer_info.flags = 0;

        if (VkContext::vk_graphics_queue_family != VkContext::vk_compute_queue_family || VkContext::vk_compute_queue_family != VkContext::vk_transfer_queue_family)
        {
            buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;

            std::array<uint32_t, 3> families = { VkContext::vk_graphics_queue_family, VkContext::vk_compute_queue_family, VkContext::vk_transfer_queue_family };
            buffer_info.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
            buffer_info.pQueueFamilyIndices = families.data();
            // Note: The original code sets sharingMode to EXCLUSIVE right after CONCURRENT. This might be a bug or intentional override.
            // I'm preserving the original logic, assuming the EXCLUSIVE override is intentional.
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else
        {
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasBuffer))
        {
            // TODO:
        }
        else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Sparse))
        {
            // TODO:
        }
        else
        {
            VmaAllocationCreateInfo alloc_info = {}; // Renamed to snake_case
            alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

            switch (desc.Usage)
            {
            case Usage::ReadBack:
                alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case Usage::Upload:
                alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case Usage::Dynamic:
                alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case Usage::Default:
            default:
                break;
            }

            if (desc.Alias == nullptr)
            {
                vulkan_check(
                    vmaCreateBuffer(VkContext::vma_allocator, &buffer_info, &alloc_info, &impl.vk_buffer, &impl.allocation, nullptr));
            }
            else
            {
                // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
                if (std::holds_alternative<TextureHandle>(desc.Alias->handle))
                {
#if false
                    // This section is commented out in the original code, keeping it commented.
                    // If uncommented, they would need VkContext:: prefix for m_texturePool and VkResult res.
                    // Texture_VK* alias_texture = VkContext::texture_pool.Get(std::get<TextureHandle>(desc.Alias->Handle)); // Renamed to snake_case
                    // res = vmaCreateAliasingBuffer2(
                    //     VkContext::vma_allocator,
                    //     alias_texture->Allocation,
                    //     desc.Alias->AliasOffset,
                    //     &buffer_info,
                    //     &impl.BufferVk);
#else
                    PHX_CORE_ASSERT(false, "TODO");
#endif
                }
                else
                {
                    Buffer_VK* alias_buffer = VkContext::buffer_pool.GetHot(std::get<GpuBufferHandle>(desc.Alias->handle)); // Renamed to snake_case
                    assert(alias_buffer);

                    vulkan_check(
                        vmaCreateAliasingBuffer2(
                            VkContext::vma_allocator,
                            alias_buffer->allocation,
                            desc.Alias->offset,
                            &buffer_info,
                            &impl.vk_buffer));

                }
            }

#ifdef PHX_DEBUG
            // Now you have allocInfo.memoryType, which tells you which memory type was used
            VkPhysicalDeviceMemoryProperties memory_properties; // Renamed to snake_case
            vkGetPhysicalDeviceMemoryProperties(VkContext::vk_chosen_physical_device, &memory_properties);

            // Use the memoryTypeIndex to find the memory type
            VkMemoryType memory_type = memory_properties.memoryTypes[impl.allocation->GetMemoryTypeIndex()]; // Renamed to snake_case

            // Find the corresponding heap
            uint32_t heap_index = memory_type.heapIndex; // Renamed to snake_case
            VkMemoryHeap heap = memory_properties.memoryHeaps[heap_index]; // Renamed to snake_case

            VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
            vmaGetHeapBudgets(VkContext::vma_allocator, budgets);

            PHX_CORE_INFO("[Vulkan] Created Buffer on {0} - {1}/{2}", heap_index, budgets[heap_index].usage, heap.size);
#endif
        }

        if (desc.Usage == Usage::ReadBack || desc.Usage == Usage::Upload || desc.Usage == Usage::Dynamic)
        {
            impl.mapped_data = impl.allocation->GetMappedData();
            impl.mapped_data_size = impl.allocation->GetSize();
        }

        if (buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            info.buffer = impl.vk_buffer;
            impl.gpu_address = vkGetBufferDeviceAddress(VkContext::vk_device, &info);
        }

        if (initial_data) // Assuming initial_data is a parameter to this function
        {
            RHI::vk::CopyCtx copy_ctx;
            Buffer_VK* copy_buffer;
            void* mapped_data = nullptr;
            if (desc.Usage == Usage::Upload)
            {
                mapped_data = impl.mapped_data;
            }
            else
            {
                copy_ctx = VkContext::copy_ctx_manager.Allocate(impl.allocation->GetSize());
                copy_buffer = VkContext::buffer_pool.GetHot(copy_ctx.upload_buffer);
                mapped_data = copy_buffer->mapped_data;
            }

            std::memcpy(mapped_data, initial_data, impl.allocation->GetSize());

            if (copy_ctx.IsValid())
            {
                VkBufferCopy copy_region = {}; // Renamed to snake_case
                copy_region.size = desc.Size;
                copy_region.srcOffset = 0;
                copy_region.dstOffset = 0;

                vkCmdCopyBuffer(
                    copy_ctx.transfer_command_buffer,
                    copy_buffer->vk_buffer,
                    impl.vk_buffer,
                    1,
                    &copy_region
                );

                VkBufferMemoryBarrier2 barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                barrier.buffer = impl.vk_buffer;
                barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                barrier.size = VK_WHOLE_SIZE;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ConstantBuffer))
                {
                    barrier.dstAccessMask |= VK_ACCESS_2_UNIFORM_READ_BIT;
                }
                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::VertexBuffer))
                {
                    barrier.dstStageMask |= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
                    barrier.dstAccessMask |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
                }
                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::IndexBuffer))
                {
                    barrier.dstStageMask |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
                    barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
                }
                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ShaderResource))
                {
                    barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
                }
                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess))
                {
                    barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
                    barrier.dstAccessMask |= VK_ACCESS_2_SHADER_WRITE_BIT;
                }
                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::IndirectBuffer))
                {
                    barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
                }
                if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::RayTracing))
                {
                    barrier.dstAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                }

                VkDependencyInfo dependency_info = {}; // Renamed to snake_case
                dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dependency_info.bufferMemoryBarrierCount = 1;
                dependency_info.pBufferMemoryBarriers = &barrier;

                vkCmdPipelineBarrier2(copy_ctx.transition_command_buffer, &dependency_info);

                VkContext::copy_ctx_manager.SubmitAndWait(copy_ctx);
            }
        }

        if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
        {
            CreateSubResource(impl, desc, SubresouceType::SRV, 0u);
        }

        if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
        {
            CreateSubResource(impl, desc, SubresouceType::UAV, 0u);
        }

        return ret_val;
	}

	TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData)
	{
        PHX_PROFILE_SECTION("Vulkan::PlatformCreateTexture");
        PHX_CORE_WARN("[RHI] PlatformCreateTexture - Not Implemented");
        return {};
	}

	PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc)
	{
        PHX_PROFILE_SECTION("Vulkan::PlatformCreatePipeline");
        PHX_CORE_WARN("[RHI] PlatformCreatePipeline - Not Implemented");
        return {};
	}

    void DeletePipeline(PipelineStateHandle /*handle*/)
    {
        VkContext::EnqueueDelete({
            VkContext::frame_number,
            []()
            {
                PHX_PROFILE_SECTION("Vulkan::PlatformDeletePipeline");
                PHX_CORE_WARN("[RHI] PlatformDeletePipeline (Handle: {0}) - Not Implemented");
            }
            });
    }

    void DeleteTexture(TextureHandle /*handle*/)
    {
        VkContext::EnqueueDelete({
            VkContext::frame_number,
            []()
            {
                PHX_PROFILE_SECTION("Vulkan::PlatformDeleteTexture");
                PHX_CORE_WARN("[RHI] PlatformDeleteTexture (Handle: {0}) - Not Implemented");
            }
            });
    }

    void DeleteBuffer(GpuBufferHandle handle)
    {
        VkContext::EnqueueDelete({
            VkContext::frame_number,
            [handle]()
            {
                Buffer_VK* impl = VkContext::buffer_pool.GetHot(handle);
                // TODO: Move into the deconstructor of struct
                if (impl)
                {
#if false
                    // These sections are commented out in the original code, keeping them commented.
                    // If uncommented, they would need VkContext:: prefix for the pools and VkContext::vk_device.
                    // if (impl->Srv.IsValid())
                    // {
                    //     if (impl->Srv.IsTyped)
                    //     {
                    //         VkContext::bindless_uniform_texel_buffers.Free(impl->Srv.Index);
                    //     }
                    //     else
                    //     {
                    //         VkContext::bindless_storage_buffers.Free(impl->Srv.Index);
                    //     }
                    //
                    //     if (impl->Srv.ViewVk != VK_NULL_HANDLE)
                    //         vkDestroyBufferView(VkContext::vk_device, impl->Uav.ViewVk, nullptr);
                    //     impl->Srv = {};
                    // }
                    // if (impl->Uav.IsValid())
                    // {
                    //     if (impl->Uav.IsTyped)
                    //     {
                    //         VkContext::bindless_storage_texel_buffers.Free(impl->Uav.Index);
                    //     }
                    //     else
                    //     {
                    //         VkContext::bindless_storage_buffers.Free(impl->Uav.Index);
                    //     }
                    //
                    //     if (impl->Uav.ViewVk != VK_NULL_HANDLE)
                    //         vkDestroyBufferView(VkContext::vk_device, impl->Uav.ViewVk, nullptr);
                    //     impl->Uav = {};
                    // }
#endif
                    // TODO: Descriptors
                    // TODO: Free Views
                    if (impl->buffer_view != VK_NULL_HANDLE)
                        vkDestroyBufferView(VkContext::vk_device, impl->buffer_view, nullptr);

                    vmaDestroyBuffer(VkContext::vma_allocator, impl->vk_buffer, impl->allocation);
                }

                VkContext::buffer_pool.Free(handle);
            }
            });
    }
	DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type)
	{
		return {};
	}

	Budget GetBudget()
	{
		return {};
	}

	ShaderFormat GetShaderFormat()
	{
		return ShaderFormat::Spirv;
	}

}

namespace phx::RHI::CommandRecorder
{
    void BindPipelineState(CommandBufferHandle handle, PipelineStateHandle pso)
    {
    }

    void Draw(CommandBufferHandle handle, uint32_t vertex_count, uint32_t start_vertex_location)
    {
        CommandBuffer_VK& cmd_buffer = VkContext::command_buffers[handle];
        vkCmdDraw(
            cmd_buffer.vk_cmd_buffer,
            vertex_count,
            1,
            start_vertex_location,
            0);
    }

    void DrawIndexed(CommandBufferHandle handle, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
    {
        CommandBuffer_VK& cmd_buffer = VkContext::command_buffers[handle];
        vkCmdDrawIndexed(
            cmd_buffer.vk_cmd_buffer,
            index_count,
            1,
            start_index_location,
            base_vertex_location,
            0);
    }

    void DrawInstanced(CommandBufferHandle handle, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
    {
        CommandBuffer_VK& cmd_buffer = VkContext::command_buffers[handle];
        vkCmdDraw(
            cmd_buffer.vk_cmd_buffer,
            vertex_count,
            instance_count,
            start_vertex_location,
            start_instance_location);
    }

    void DrawIndexedInstanced(CommandBufferHandle handle, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
    {
        CommandBuffer_VK& cmd_buffer = VkContext::command_buffers[handle];
        vkCmdDrawIndexed(
            cmd_buffer.vk_cmd_buffer,
            index_count,
            instance_count,
            start_index_location,
            base_vertex_location,
            start_instance_location);
    }
}