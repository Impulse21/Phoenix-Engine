#include "PhxRhi/PhxRhi_pch.h"

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

#include <PhxCore/Memory/IAllocator.h>
#include <PhxCore/EnumUtils.h>

#include "VkGfxDevice.h"
#include "VkCommandCtx.h"

#include <PhxCore/Log.h>
#include <PhxCore/Memory/MemoryUtils.h>
#include <PhxCore/Memory/IAllocator.h>

#include "VkTypes.h"


#define VOLK_IMPLEMENTATION
#include "volk.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#ifdef PHX_PLATFORM_WINDOWS
extern HINSTANCE g_hInstance;
#endif

#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_CORE_WARN("[Vulkan] - Pool '" #x "' still contains active handles"); x.Shutdown();

namespace phx::rhi::vk
{

#if USE_PHX_ALLOCATOR
    void* VKAPI_CALL VkGfxDeviceImpl::vk_phx_allocate(
        void* pUserData,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope)
    {
        auto* allocator = static_cast<phx::IAllocator*>(pUserData);
        return allocator->Allocate(size, alignment);
    }

    void* VKAPI_CALL VkGfxDeviceImpl::vk_phx_reallocate(
        void* pUserData,
        void* pOriginal,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope)
    {
        auto* allocator = static_cast<phx::IAllocator*>(pUserData);
        allocator->Deallocate(pOriginal);
        return allocator->Allocate(size, alignment);
    }

    void VKAPI_CALL VkGfxDeviceImpl::vk_phx_free(
        void* pUserData,
        void* pMemory)
    {
        auto* allocator = static_cast<phx::IAllocator*>(pUserData);
        allocator->Deallocate(pMemory);
    }
#endif

    VKAPI_ATTR VkBool32 VKAPI_CALL VkGfxDeviceImpl::vk_phx_debug_callback(
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

    VkGfxDeviceImpl::VkGfxDeviceImpl() = default;

    VkGfxDeviceImpl::~VkGfxDeviceImpl()
    {
        if (m_isInitialized)
        {
            PlatformShutdown();
        }
    }

    void VkGfxDeviceImpl::InitializeVolk()
    {
        if (!m_volkInitialized)
        {
            if (volkInitialize() != VK_SUCCESS)
            {
                PHX_CORE_ERROR("[RHI] Failed to initialize Volk.");
                return;
            }
            m_volkInitialized = true;
        }
    }

    bool VkGfxDeviceImpl::CreateInstance(const GfxDeviceDescriptor& desc)
    {
#if PHX_DEBUG
        bool useValidationLayers = true;
#else
        bool useValidationLayers = false;
#endif

        vkb::InstanceBuilder builder;
        builder.set_app_name("Phoenix RHI Application")
            .set_engine_name("PhxEngine")
            .request_validation_layers(useValidationLayers)
            .set_debug_callback(vk_phx_debug_callback)
            .set_headless(false)
            .require_api_version(1, 3, 0);

#if USE_PHX_ALLOCATOR
        m_allocCallbacks = {
            .pUserData = &phx::Memory::g_persistentAllocator,
            .pfnAllocation = vk_phx_allocate,
            .pfnReallocation = vk_phx_reallocate,
            .pfnFree = vk_phx_free,
            .pfnInternalAllocation = nullptr,
            .pfnInternalFree = nullptr
        };

        builder.set_allocation_callbacks(&m_allocCallbacks);
#endif

        auto inst_ret = builder.build();
        if (!inst_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to create Vulkan Instance: {0}", inst_ret.error().message());
            return false;
        }

        m_vkbInstance = inst_ret.value();
        m_instance = m_vkbInstance.instance;
        m_debugMessenger = m_vkbInstance.debug_messenger;
        volkLoadInstance(m_instance); // Load instance-level functions
        return true;
    }

    bool VkGfxDeviceImpl::CreateSurface(const GfxDeviceDescriptor& desc)
    {
#ifdef PHX_PLATFORM_WINDOWS
        if (!desc.WindowsHandle)
        {
            PHX_CORE_ERROR("[RHI] WindowsHandle is null in GfxDeviceDescriptor.");
            return false;
        }
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.pNext = nullptr; 
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.hwnd = static_cast<HWND>(desc.WindowsHandle);
        surfaceCreateInfo.hinstance = g_hInstance;

        VkResult result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, GetVkAllocationCallbacks(), &m_surface);
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

    bool VkGfxDeviceImpl::SelectPhysicalDevice(const GfxDeviceDescriptor&, vkb::PhysicalDevice& outVkbPhysicalDevice)
    {
        vkb::PhysicalDeviceSelector selector{ m_vkbInstance };
        VkPhysicalDeviceFeatures featuresToEnable = {}; // Populate based on desc.requiredFeatures if you add that
        featuresToEnable.samplerAnisotropy = VK_TRUE;
        // Add other features you absolutely need enabled

        const std::vector<const char*> required_extensions =
        {
            VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_KHR_MULTIVIEW_EXTENSION_NAME,
            VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME
        };

        selector.set_minimum_version(1, 3)
            .set_surface(m_surface)
            .set_required_features(featuresToEnable)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .add_required_extensions(required_extensions.size(), required_extensions.data());
        // Add specific extension requirements if vkb doesn't infer them well enough

        auto phys_dev_ret = selector.select();
        if (!phys_dev_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to select suitable Physical Device: {0}", phys_dev_ret.error().message());
            return false;
        }


        const std::vector<const char*> optional_extensions =
        {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,
            VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
            // VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
            VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
            VK_EXT_MESH_SHADER_EXTENSION_NAME,
        };
        outVkbPhysicalDevice.enable_extensions_if_present(optional_extensions);

        outVkbPhysicalDevice = phys_dev_ret.value();
        m_chosenPhysicalDevice = outVkbPhysicalDevice.physical_device;
        m_physicalDeviceProperties = outVkbPhysicalDevice.properties; // Store properties

        m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        m_features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        m_features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        m_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        m_features2.pNext       = &m_features_1_1;
        m_features_1_1.pNext    = &m_features_1_2;
        m_features_1_2.pNext    = &m_features_1_3;
        // void** features_chain   = &m_features_1_3.pNext;

        vkGetPhysicalDeviceFeatures2(m_chosenPhysicalDevice, &m_features2);
        return true;
    }

    bool VkGfxDeviceImpl::CreateLogicalDevice(const GfxDeviceDescriptor&, vkb::PhysicalDevice& vkbPhysicalDevice)
    {
        vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
        
        auto dev_ret = deviceBuilder.build();
        if (!dev_ret)
        {
            PHX_CORE_ERROR("[RHI] Failed to create Logical Device: {0}", dev_ret.error().message());
            return false;
        }

        vkb::Device vkbDevice = dev_ret.value();
        m_device = vkbDevice.device;
        volkLoadDevice(m_device); // Load device-level functions

        auto gfx_q_ret = vkbDevice.get_queue(vkb::QueueType::graphics);
        if (!gfx_q_ret) 
        { 
            PHX_CORE_ERROR("[RHI] Failed to get graphics queue: {0}", gfx_q_ret.error().message()); 
            return false; 
        }

        m_graphicsQueue = gfx_q_ret.value();
        m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        auto compute_q_ret = vkbDevice.get_queue(vkb::QueueType::compute);
        if (!compute_q_ret) 
        { 
            PHX_CORE_WARN("[RHI] Failed to get dedicated compute queue, using graphics queue: {0}", compute_q_ret.error().message());
            m_computeQueue = m_graphicsQueue;
            m_computeQueueFamily = m_graphicsQueueFamily; 
        }
        else 
        { 
            m_computeQueue = compute_q_ret.value();
            m_computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();
        }

        auto transfer_q_ret = vkbDevice.get_queue(vkb::QueueType::transfer);
        if (!transfer_q_ret)
        { 
            PHX_CORE_WARN("[RHI] Failed to get dedicated transfer queue, using graphics queue: {0}", transfer_q_ret.error().message());
            m_transferQueue = m_graphicsQueue; m_transferQueueFamily = m_graphicsQueueFamily;
        }
        else 
        { 
            m_transferQueue = transfer_q_ret.value();
            m_transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value(); 
        }

        return true;
    }

    bool VkGfxDeviceImpl::CreateAllocator(const GfxDeviceDescriptor&)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = m_chosenPhysicalDevice;
        allocatorInfo.device = m_device;
        allocatorInfo.instance = m_instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
            VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

        VkPhysicalDeviceFeatures enabledFeatures; // Need to get this from vkb::Device or query
        vkGetPhysicalDeviceFeatures(m_chosenPhysicalDevice, &enabledFeatures); // Example, better to use vkb info

        if (m_features_1_2.bufferDeviceAddress)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        VkResult res = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
        if (res != VK_SUCCESS)
        {
            PHX_CORE_ERROR("[RHI] Failed to create VMA Allocator. VkResult: <TODO>");
            return false;
        }
        return true;
    }


    bool VkGfxDeviceImpl::PlatformInitialize(GfxDeviceDescriptor const& desc)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformInitialize");
        PHX_CORE_INFO("Initializing RHI (Vulkan) - VkGfxDeviceImpl");
        m_deviceDesc = desc; // Store descriptor

        InitializeVolk();

        VkAllocationCallbacks* allocatorCallbacks = GetVkAllocationCallbacks();
        if (!CreateInstance(desc))
            return false;

        if (!CreateSurface(desc))
        {
            vkb::destroy_instance(m_vkbInstance);
            return false; 
        }

        vkb::PhysicalDevice vkbPhysicalDevice;
        if (!SelectPhysicalDevice(desc, vkbPhysicalDevice))
        { 
            vkDestroySurfaceKHR(m_instance, m_surface, allocatorCallbacks); vkb::destroy_instance(m_vkbInstance);
            return false; 
        }

        if (!CreateLogicalDevice(desc, vkbPhysicalDevice))
        { 
            vkDestroySurfaceKHR(m_instance, m_surface, allocatorCallbacks);
            vkb::destroy_instance(m_vkbInstance);
            return false; 
        }

        if (!CreateAllocator(desc)) 
        { 
            vkDestroyDevice(m_device, allocatorCallbacks);
            vkDestroySurfaceKHR(m_instance, m_surface, allocatorCallbacks);
            vkb::destroy_instance(m_vkbInstance);
            return false; 
        }

        CreateCommandPools();
        InitializeResourcePools();

        CreateSwapchain(desc); // Initial swapchain creation
        CreateFrameSyncObjects();

        m_swapchainExtent = { desc.SwapChainDesc.Width, desc.SwapChainDesc.Height };

        m_copy_ctx_manager.Initialize(this);

        m_isInitialized = true;
        PHX_CORE_INFO("[RHI] Vulkan Device Initialized Successfully.");
        return true;
    }

    void VkGfxDeviceImpl::PlatformShutdown()
    {
        if (!m_isInitialized)
        {
            return;
        }
        PHX_PROFILE_SECTION("Vulkan::PlatformShutdown");
        PHX_CORE_INFO("Shutting down RHI (Vulkan) - VkGfxDeviceImpl");

        PlatformWaitForIdle();

        m_copy_ctx_manager.Shutdown();

        ProcessDeletionQueue(UINT64_MAX);

        DestroyFrameSyncObjects();
        CleanupSwapchain();

        ShutdownResourcePools();

        DestroyCommandPools();

        if (m_vmaAllocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(m_vmaAllocator);
            m_vmaAllocator = VK_NULL_HANDLE;
        }

        if (m_device != VK_NULL_HANDLE) // vkb::Device doesn't have a destructor, must be explicit if not member
        {
            vkDestroyDevice(m_device, GetVkAllocationCallbacks());
            m_device = VK_NULL_HANDLE;
        }

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, GetVkAllocationCallbacks());
            m_surface = VK_NULL_HANDLE;
        }

        // m_vkbInstance's destructor will handle VkInstance and debug messenger
        // No explicit call to vkb::destroy_instance(m_vkbInstance) needed if m_vkbInstance is a member
        m_instance = VK_NULL_HANDLE; // Its destructor handles it
        m_debugMessenger = VK_NULL_HANDLE;


        m_isInitialized = false;
        PHX_CORE_INFO("[RHI] Vulkan Device Shutdown Complete.");
    }

    void VkGfxDeviceImpl::CreateSwapchain(const GfxDeviceDescriptor& desc)
    {
        PHX_PROFILE_SECTION("Vulkan::CreateSwapchain");
        vkb::SwapchainBuilder swapchainBuilder(m_chosenPhysicalDevice, m_device, m_surface);

        auto swap_ret = swapchainBuilder
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

        vkb::Swapchain vkbSwapchain = swap_ret.value();
        m_swapchain = vkbSwapchain.swapchain;
        m_swapchainImageFormat = vkbSwapchain.image_format;
        m_swapchainExtent = vkbSwapchain.extent;

        m_swapchainImages = vkbSwapchain.get_images().value();
        m_swapchainImageViews = vkbSwapchain.get_image_views().value();

        PHX_CORE_INFO(
            "[RHI] Swapchain Initialized. Extent: {0}x{1}, Format: {2}, Images: {3}",
            m_swapchainExtent.width,
            m_swapchainExtent.height,
            "",
            m_swapchainImages.size());
    }

    void VkGfxDeviceImpl::RecreateSwapchain(const GfxDeviceDescriptor& desc)
    {
        PlatformWaitForIdle();
        CleanupSwapchain();
        CreateSwapchain(desc);
        // Potentially need to recreate framebuffers if they depend on swapchain image views
    }

    void VkGfxDeviceImpl::CleanupSwapchain()
    {
        if (m_device == VK_NULL_HANDLE) 
            return;

        for (auto imageView : m_swapchainImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_device, imageView, GetVkAllocationCallbacks());
            }
        }
        m_swapchainImageViews.clear();
        m_swapchainImages.clear();

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, GetVkAllocationCallbacks());
            m_swapchain = VK_NULL_HANDLE;
        }
    }

    void VkGfxDeviceImpl::CreateFrameSyncObjects()
    {
        PHX_PROFILE_SECTION("Vulkan::CreateFrameSyncObjects");
        VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < cMaxInflightFrames; ++i)
        {
        	VkResult result = vkCreateSemaphore(m_device, &semaphoreInfo, GetVkAllocationCallbacks(), &m_frames[i].PresentSemaphore);
            PHX_CORE_ASSERT(result == VK_SUCCESS);

            result = vkCreateSemaphore(m_device, &semaphoreInfo, GetVkAllocationCallbacks(), &m_frames[i].RenderSemaphore);
            PHX_CORE_ASSERT(result == VK_SUCCESS);

            result = vkCreateFence(m_device, &fenceInfo, GetVkAllocationCallbacks(), &m_frames[i].RenderFence);
            PHX_CORE_ASSERT(result == VK_SUCCESS);
        }

        PHX_CORE_INFO("[RHI] Frame synchronization primitives created.");
    }

    void VkGfxDeviceImpl::DestroyFrameSyncObjects()
    {
        if (m_device == VK_NULL_HANDLE) return;
        for (size_t i = 0; i < cMaxInflightFrames; ++i)
        {
            if (m_frames[i].RenderFence != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_device, m_frames[i].RenderFence, GetVkAllocationCallbacks());
                m_frames[i].RenderFence = VK_NULL_HANDLE;
            }
            if (m_frames[i].RenderSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_device, m_frames[i].RenderSemaphore, GetVkAllocationCallbacks());
                m_frames[i].RenderSemaphore = VK_NULL_HANDLE;
            }
            if (m_frames[i].PresentSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_device, m_frames[i].PresentSemaphore, GetVkAllocationCallbacks());
                m_frames[i].PresentSemaphore = VK_NULL_HANDLE;
            }
        }
        PHX_CORE_INFO("[RHI] Frame synchronization primitives destroyed.");
    }

    void VkGfxDeviceImpl::CreateCommandPools()
    {
        PHX_PROFILE_SECTION("Vulkan::CreateCommandPools");

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VkResult result = vkCreateCommandPool(m_device, &poolInfo, GetVkAllocationCallbacks(), &m_graphicsCommandPool);

        PHX_CORE_ASSERT(result == VK_SUCCESS);
        PHX_CORE_INFO("[RHI] Graphics Command Pool created.");
        // Create other command pools (compute, transfer) if needed
    }

    void VkGfxDeviceImpl::DestroyCommandPools()
    {
        if (m_graphicsCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_graphicsCommandPool, GetVkAllocationCallbacks());
            m_graphicsCommandPool = VK_NULL_HANDLE;
            PHX_CORE_INFO("[RHI] Graphics Command Pool destroyed.");
        }
    }

    void VkGfxDeviceImpl::InitializeResourcePools()
    {
        // TODO: Data drive these
        m_bufferPool.Initialize(4096);
    }

    void VkGfxDeviceImpl::ShutdownResourcePools()
    {
        LOG_AND_SHUTDOWN_POOL(m_bufferPool);
    }

    int VkGfxDeviceImpl::CreateSubResource(Buffer_VK& buffer, GpuBufferDescriptor const& desc, SubresouceType subresourceType, size_t offset, size_t size)
    {
        assert(subresourceType == SubresouceType::SRV || subresourceType == SubresouceType::UAV);

        Format format = desc.Format;

        // Is raw buffer
        if (format == Format::UNKNOWN)
        {
            buffer.srv_is_typed = false;
            // buffer.srv_index = m_bindlessStorageBuffers.Allocate();

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = buffer.vk_buffer;
            bufferInfo.offset = offset;
            bufferInfo.range = size;

            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.dstBinding = 0;
            write.dstArrayElement = buffer.srv_index;
            write.descriptorCount = 1;
            //write.dstSet = m_bindlessStorageBuffers.DescritporSetVk;
            write.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }
        else
        {
            // Typed buffer
            buffer.srv_is_typed = true;

            VkBufferViewCreateInfo srvDesc = {};
            srvDesc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            srvDesc.buffer = buffer.vk_buffer;
            srvDesc.flags = 0;
            srvDesc.format = FormatToVkFormat(format);
            srvDesc.offset = offset;
            srvDesc.range = std::min(size, (uint64_t)desc.Size - srvDesc.offset);

            VkResult res = vkCreateBufferView(m_device, &srvDesc, nullptr, &buffer.buffer_view);
            assert(res == VK_SUCCESS);

            if (subresourceType == SubresouceType::SRV)
            {
                // buffer.srv_index = m_bindlessUniformTexelBuffers.Allocate();
                if (buffer.buffer_view != VK_NULL_HANDLE)
                {
                    VkWriteDescriptorSet write = {};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                    write.dstBinding = 0;
                    write.dstArrayElement = buffer.srv_index;
                    write.descriptorCount = 1;
                    // write.dstSet = m_bindlessUniformTexelBuffers.DescritporSetVk;
                    write.pTexelBufferView = &buffer.buffer_view;
                    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
                }

                return -1;
            }
            else
            {
                // buffer.uav_index = m_bindlessStorageTexelBuffers.Allocate();
                if (buffer.buffer_view != VK_NULL_HANDLE)
                {
                    VkWriteDescriptorSet write = {};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                    write.dstBinding = 0;
                    write.dstArrayElement = buffer.uav_index;
                    write.descriptorCount = 1;
                    // write.dstSet = m_bindlessStorageTexelBuffers.DescritporSetVk;
                    write.pTexelBufferView = &buffer.buffer_view;
                    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
                }
                return -1;
            }
        }

        return 0;
    }

    phx::rhi::vk::VkCommandCtxImpl* VkGfxDeviceImpl::PlatformBeginCommandBuffer(phx::IAllocator* frame_arena)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformBeginCommandBuffer");
        if (!m_isInitialized || m_graphicsCommandPool == VK_NULL_HANDLE)
        {
            PHX_CORE_ERROR("[RHI] Cannot begin command buffer: RHI not initialized or no command pool.");
            return nullptr;
        }

        FrameData& currentFrame = GetCurrentFrameData();

        VkResult waitResult = vkWaitForFences(m_device, 1, &currentFrame.RenderFence, VK_TRUE, UINT64_MAX);
        PHX_CORE_ASSERT(waitResult); // Check for VK_TIMEOUT or errors
        PHX_CORE_ASSERT(vkResetFences(m_device, 1, &currentFrame.RenderFence));

        // Acquire next image before starting new command buffer that might use it
        VkResult acquireResult = vkAcquireNextImageKHR(
            m_device,
            m_swapchain,
            UINT64_MAX,
            currentFrame.PresentSemaphore,
            VK_NULL_HANDLE,
            &m_swapchainImageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
        {
            PHX_CORE_WARN("[RHI] Swapchain out of date or suboptimal during acquire. Recreation needed.");
            RecreateSwapchain(m_deviceDesc); // Pass stored descriptor
            // Try acquiring again (or handle failure more gracefully)
            acquireResult = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, currentFrame.PresentSemaphore, VK_NULL_HANDLE, &m_swapchainImageIndex);
            if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) // VK_SUBOPTIMAL_KHR is okay to continue with
            {
                PHX_CORE_ERROR("[RHI] Failed to acquire swap chain image after recreation! VkResult: <TODO>");
                return nullptr;
            }
        }
        else if (acquireResult != VK_SUCCESS)
        {
            PHX_CORE_ERROR("[RHI] Failed to acquire swap chain image! VkResult: <TODO>");
            return nullptr;
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_graphicsCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer vkCmdBuffer;
        PHX_CORE_ASSERT(vkAllocateCommandBuffers(m_device, &allocInfo, &vkCmdBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        PHX_CORE_ASSERT(vkBeginCommandBuffer(vkCmdBuffer, &beginInfo));

        VkCommandCtxImpl* cmdCtx = frame_arena->NewObject<VkCommandCtxImpl>();
        cmdCtx->PlatfomrInitialize(vkCmdBuffer, this, m_swapchainImageIndex);

        return cmdCtx;
    }

    void VkGfxDeviceImpl::PlatformSubmitFrame(VkCommandCtxImpl* cmdCtx)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformSubmitFrame");
        if (!cmdCtx|| !m_isInitialized) 
            return;

        VkCommandBuffer vkCmdBuffer = cmdCtx->GetVkCommandBuffer(); // Assume method exists

        PHX_CORE_ASSERT(vkEndCommandBuffer(vkCmdBuffer)); // End command buffer before submit

        FrameData& currentFrame = GetCurrentFrameData();

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { currentFrame.PresentSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCmdBuffer;

        VkSemaphore signalSemaphores[] = { currentFrame.RenderSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        PHX_CORE_ASSERT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, currentFrame.RenderFence));

        // After submission, the command buffer from the pool can often be freed or reset
        // depending on your VkCommandCtxImpl's lifetime management.
        // For a one-shot command buffer like this, if not pooled:
        // vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &vkCmdBuffer);
        // delete cmdCtx; // If newed directly in PlatformBeginCommandBuffer and not pooled
    }


    void VkGfxDeviceImpl::PlatformPresent()
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformPresent");
        if (!m_isInitialized || m_swapchain == VK_NULL_HANDLE) return;

        FrameData& currentFrame = GetCurrentFrameData();

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &currentFrame.RenderSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_swapchainImageIndex;
        presentInfo.pResults = nullptr;

        VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            PHX_CORE_WARN("[RHI] Swapchain is out of date or suboptimal during present. Attempting recreation.");
            RecreateSwapchain(m_deviceDesc);
        }
        else if (presentResult != VK_SUCCESS)
        {
            PHX_CORE_ERROR("[RHI] Failed to present swap chain image. VkResult: <TODO>");
        }
        m_frameNumber++;
    }

    void VkGfxDeviceImpl::PlatformWaitForIdle()
    {
        PHX_CORE_ASSERT(m_isInitialized && m_device != VK_NULL_HANDLE);
    	vkDeviceWaitIdle(m_device);
    }

    GpuBufferHandle VkGfxDeviceImpl::PlatformCreateBuffer(const GpuBufferDescriptor& desc, const void* initial_data)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformCreateBuffer");
        if (!m_isInitialized || m_vmaAllocator == VK_NULL_HANDLE) return GpuBufferHandle();

        Handle<GpuBuffer> retVal = m_bufferPool.Allocate();
        Buffer_VK& impl = *this->m_bufferPool.GetHot(retVal);

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = desc.Size;
        bufferInfo.usage = 0;


        static const std::vector <std::pair<BindingFlags, VkBufferUsageFlags>> kUsageMapping =
        {
            { BindingFlags::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
            { BindingFlags::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
            { BindingFlags::ConstantBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
            { BindingFlags::ShaderResource, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT},
            { BindingFlags::UnorderedAccess, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT},
        };

        for (const auto& [flag, usageFlag] : kUsageMapping)
        {
            if (phx::EnumHasAnyFlags(desc.BindingFlags, flag))
            {
                bufferInfo.usage |= usageFlag;
            }
        }

        // Misc Flags
        static const std::vector <std::pair<ResourceMiscFlags, VkBufferUsageFlags>> kUsageMappingMisc =
        {
            { ResourceMiscFlags::BufferRaw, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
            { ResourceMiscFlags::BufferStructured, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
            { ResourceMiscFlags::IndirectArgs, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},
            { ResourceMiscFlags::RayTracing, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR },
        };

        for (const auto& [flag, usageFlag] : kUsageMappingMisc)
        {
            if (EnumHasAnyFlags(desc.MiscFlags, flag))
            {
                bufferInfo.usage |= usageFlag;
            }
        }

        if (m_features_1_2.bufferDeviceAddress == VK_TRUE)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }

        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        bufferInfo.flags = 0;

        if (m_graphicsQueueFamily != m_computeQueueFamily != m_transferQueueFamily)
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

            std::array<uint32_t, 3> families = { m_graphicsQueueFamily, m_computeQueueFamily, m_transferQueueFamily };
            bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
            bufferInfo.pQueueFamilyIndices = families.data();
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
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
            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            switch (desc.Usage)
            {
            case Usage::ReadBack:
                allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case Usage::Upload:
                allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case Usage::Dynamic:
                allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case Usage::Default:
            default:
                break;
            }

            VkResult res = VK_SUCCESS;
            if (desc.Alias == nullptr)
            {
                res = vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &impl.vk_buffer, &impl.allocation, nullptr);
            }
            else
            {
                // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
                if (std::holds_alternative<TextureHandle>(desc.Alias->handle))
                {
#if false
                    Texture_VK* aliasTexture = m_texturePool.Get(std::get<TextureHandle>(desc.Alias->Handle));
                    res = vmaCreateAliasingBuffer2(
                        m_vmaAllocator,
                        aliasTexture->Allocation,
                        desc.Alias->AliasOffset,
                        &bufferInfo,
                        &impl.BufferVk);
#else
                    PHX_CORE_ASSERT(false, "TODO");
#endif
                }
                else
                {
                    Buffer_VK* aliasBuffer = m_bufferPool.GetHot(std::get<GpuBufferHandle>(desc.Alias->handle));
                    assert(aliasBuffer);
                    res = vmaCreateAliasingBuffer2(
                        m_vmaAllocator,
                        aliasBuffer->allocation,
                        desc.Alias->offset,
                        &bufferInfo,
                        &impl.vk_buffer);

                }
            }

#ifdef PHX_DEBUG
            // Now you have allocInfo.memoryType, which tells you which memory type was used
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(m_chosenPhysicalDevice, &memoryProperties);

            // Use the memoryTypeIndex to find the memory type
            VkMemoryType memoryType = memoryProperties.memoryTypes[impl.allocation->GetMemoryTypeIndex()];

            // Find the corresponding heap
            uint32_t heapIndex = memoryType.heapIndex;
            VkMemoryHeap heap = memoryProperties.memoryHeaps[heapIndex];

            VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
            vmaGetHeapBudgets(m_vmaAllocator, budgets);

            PHX_CORE_INFO("[Vulkan] Created Buffer on {0} - {1}/{2}", heapIndex, budgets[heapIndex].usage, heap.size);
#endif
        }

        if (desc.Usage == Usage::ReadBack || desc.Usage == Usage::Upload || desc.Usage == Usage::Dynamic)
        {
            impl.mapped_data = impl.allocation->GetMappedData();
            impl.mapped_data_size= impl.allocation->GetSize();
        }

        if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            info.buffer = impl.vk_buffer;
            impl.gpu_address = vkGetBufferDeviceAddress(m_device, &info);
        }

        if (initial_data)
        {
            CopyCtx copy_ctx;
            Buffer_VK* copy_buffer;
            void* mapped_data = nullptr;
            if (desc.Usage == Usage::Upload)
            {
                mapped_data = impl.mapped_data;
            }
            else
            {
                copy_ctx = m_copy_ctx_manager.Allocate(impl.allocation->GetSize());
                copy_buffer = m_bufferPool.GetHot(copy_ctx.upload_buffer);
                mapped_data = copy_buffer->mapped_data;
            }

            // TODO: Set mapped data
            if (copy_ctx.IsValid())
            {
                VkBufferCopy copyRegion = {};
                copyRegion.size = desc.Size;
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = 0;

                vkCmdCopyBuffer(
                    copy_ctx.transfer_command_buffer,
                    copy_buffer->vk_buffer,
                    impl.vk_buffer,
                    1,
                    &copyRegion
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

                VkDependencyInfo dependencyInfo = {};
                dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dependencyInfo.bufferMemoryBarrierCount = 1;
                dependencyInfo.pBufferMemoryBarriers = &barrier;

                vkCmdPipelineBarrier2(copy_ctx.transition_command_buffer, &dependencyInfo);

                m_copy_ctx_manager.SubmitAndWait(copy_ctx);
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
    }

    TextureHandle VkGfxDeviceImpl::PlatformCreateTexture(const TextureDescriptor& desc, const void* initialData)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformCreateTexture");
        PHX_CORE_WARN("[RHI] PlatformCreateTexture - Not Implemented");
        return {};
    }

    PipelineStateHandle VkGfxDeviceImpl::PlatformCreatePipeline(const PipelineStateDescriptor& desc)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformCreatePipeline");
        PHX_CORE_WARN("[RHI] PlatformCreatePipeline - Not Implemented");
        return {};
    }

    void VkGfxDeviceImpl::PlatformDeletePipeline(PipelineStateHandle /*handle*/)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformDeletePipeline");
        PHX_CORE_WARN("[RHI] PlatformDeletePipeline (Handle: {0}) - Not Implemented");
    }

    void VkGfxDeviceImpl::PlatformDeleteTexture(TextureHandle /*handle*/)
    {
        PHX_PROFILE_SECTION("Vulkan::PlatformDeleteTexture");
        PHX_CORE_WARN("[RHI] PlatformDeleteTexture (Handle: {0}) - Not Implemented");
    }

    void VkGfxDeviceImpl::PlatformDeleteBuffer(GpuBufferHandle handle)
    {
        Buffer_VK* impl = m_bufferPool.GetHot(handle);
        // TODO: Move into the deconstructor of struct
        if (impl)
        {
#if false
            if (impl->Srv.IsValid())
            {
                if (impl->Srv.IsTyped)
                {
                    m_bindlessUniformTexelBuffers.Free(impl->Srv.Index);
                }
                else
                {
                    m_bindlessStorageBuffers.Free(impl->Srv.Index);
                }

                if (impl->Srv.ViewVk != VK_NULL_HANDLE)
                    vkDestroyBufferView(m_vkDevice, impl->Uav.ViewVk, nullptr);
                impl->Srv = {};
            }
            if (impl->Uav.IsValid())
            {
                if (impl->Uav.IsTyped)
                {
                    m_bindlessStorageTexelBuffers.Free(impl->Uav.Index);
                }
                else
                {
                    m_bindlessStorageBuffers.Free(impl->Uav.Index);
                }

                if (impl->Uav.ViewVk != VK_NULL_HANDLE)
                    vkDestroyBufferView(m_vkDevice, impl->Uav.ViewVk, nullptr);
                impl->Uav = {};
            }
#endif
            // TODO: Descriptors
            // TODO: Free Views
            if (impl->buffer_view != VK_NULL_HANDLE)
                vkDestroyBufferView(m_device, impl->buffer_view, nullptr);

            vmaDestroyBuffer(m_vmaAllocator, impl->vk_buffer, impl->allocation);
        }

        m_bufferPool.Free(handle);
    }

    DescriptorIndex VkGfxDeviceImpl::PlatformGetDescriptorIndex(TextureHandle /*handle*/, SubresouceType /*type*/) const
    {
        PHX_CORE_WARN("[RHI] PlatformGetDescriptorIndex (Handle: {0}) - Not Implemented");
        return DescriptorIndex();
    }

    ShaderFormat VkGfxDeviceImpl::PlatformGetShaderFormat() const
    {
        return ShaderFormat::Spirv;
    }

    Budget VkGfxDeviceImpl::PlatformGetBudget() const
    {
        if (!m_isInitialized || m_vmaAllocator == VK_NULL_HANDLE)
            return {};

#if false
        vmaTotalBudget budgets[VK_MAX_MEMORY_HEAPS]; // Changed to VmaTotalBudget
        vmaGetBudget(m_vmaAllocator, budgets); // Changed to vmaGetBudget

        Budget rhiBudget = {};
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_chosenPhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
        {
            if (memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                rhiBudget.TotalBytes += budgets[i].budget;
                rhiBudget.UsedBytes += budgets[i].usage;
            }
        }
        return rhiBudget;
#else
        return {};
#endif
    }
}