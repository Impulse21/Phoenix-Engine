#pragma once

#include "VkCommon.h"

#include <PhxCore/Pool.h>
#include "VkTypes.h"
#include "VkCopyCtxManager.h"

namespace phx::rhi::vk
{
    class VkCommandCtxImpl;
}

namespace phx::rhi::vk
{
    constexpr size_t cMaxInflightFrames = 2;

    struct FrameData
    {
        VkSemaphore PresentSemaphore = VK_NULL_HANDLE;
        VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
        VkFence RenderFence = VK_NULL_HANDLE;
        // Each frame might have its own command pool if desired for multi-threaded recording
        // VkCommandPool CommandPool = VK_NULL_HANDLE; 
        // std::vector<VkCommandBuffer> CommandBuffers; // If pre-allocating
    };

    class VkGfxDeviceImpl : public BaseGfxDevice<VkGfxDeviceImpl>
    {
        friend class BaseGfxDevice<VkGfxDeviceImpl>;

    public:
        VkGfxDeviceImpl();
        ~VkGfxDeviceImpl();

        bool PlatformInitialize(GfxDeviceDescriptor const& desc);
        void PlatformShutdown();

        phx::rhi::vk::VkCommandCtxImpl* PlatformBeginCommandBuffer(phx::IAllocator* frame_arena);

        void PlatformPresent();
        void PlatformSubmitFrame(VkCommandCtxImpl* cmdCtx); // Added for clarity

        void PlatformWaitForIdle();

        GpuBufferHandle PlatformCreateBuffer(const GpuBufferDescriptor& desc, const void* initialData = nullptr);
        TextureHandle PlatformCreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr);
        PipelineStateHandle PlatformCreatePipeline(const PipelineStateDescriptor& desc);

        void PlatformDeletePipeline(PipelineStateHandle handle);
        void PlatformDeleteTexture(TextureHandle handle);
        void PlatformDeleteBuffer(GpuBufferHandle handle);

        DescriptorIndex PlatformGetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV) const;

        VkDevice GetLogicalDevice() const { return m_device; }

        ShaderFormat PlatformGetShaderFormat() const;
        Budget PlatformGetBudget() const;

        VkDevice GetVkDevice() const { return m_device; }
        VmaAllocator GetVmaAllocator() const { return m_vmaAllocator; }

        VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
        uint32_t GetGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
        std::mutex& GetGraphicsQueueLock() { return m_graphicsQueueLock; }

        VkQueue GetTransferQueue() const { return m_transferQueue; }
        uint32_t GetTransferQueueFamily() const { return m_transferQueueFamily; }
        std::mutex& GetTransferQueueLock() { return m_transferQueueLock; }

        VkFormat GetSwapchainImageFormat() const { return m_swapchainImageFormat; }
        VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }
        VkRenderPass GetDefaultRenderPass() const { return VK_NULL_HANDLE; } // Placeholder for actual default render pass

        Buffer_VK* GetResourceInternal(GpuBufferHandle handle)
        {
            return m_bufferPool.GetHot(handle);
        }
    private:

        FrameData& GetCurrentFrameData() { return m_frames[m_frameNumber % cMaxInflightFrames]; }

        void InitializeVolk();

        bool CreateInstance(const GfxDeviceDescriptor& desc);
        bool CreateSurface(const GfxDeviceDescriptor& desc);

        bool SelectPhysicalDevice(const GfxDeviceDescriptor& desc, vkb::PhysicalDevice& outVkbPhysicalDevice);

        bool CreateLogicalDevice(const GfxDeviceDescriptor& desc, vkb::PhysicalDevice& vkbPhysicalDevice);
        bool CreateAllocator(const GfxDeviceDescriptor& desc);

        void CreateSwapchain(const GfxDeviceDescriptor& desc); // Changed from InitSwapchain
        void RecreateSwapchain(const GfxDeviceDescriptor& desc);
        void CleanupSwapchain(); // Changed from DestroySwapchain

        void CreateFrameSyncObjects(); // Changed from CreateFrameData
        void DestroyFrameSyncObjects(); // Changed from DestroyFrameData

        void CreateCommandPools();
        void DestroyCommandPools();

        void InitializeResourcePools();
        void ShutdownResourcePools();

        int CreateSubResource(Buffer_VK& buffer, GpuBufferDescriptor const& desc, SubresouceType subresourceType, size_t offset, size_t size = ~0u);


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
        static VKAPI_ATTR VkBool32 VKAPI_CALL vk_phx_debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

    private:
        bool m_isInitialized = false;
        bool m_volkInitialized = false;
        size_t m_frameNumber = 0;
        GfxDeviceDescriptor m_deviceDesc;
        DeviceCapability m_capabilities = {};

        CopyCtxManager m_copy_ctx_manager = {};

        // -- VK Core ---
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_chosenPhysicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_physicalDeviceProperties = {};
        VkPhysicalDeviceFeatures m_physicalDeviceFeatures = {};

        VkPhysicalDeviceFeatures2 m_features2 = {};
        VkPhysicalDeviceVulkan11Features m_features_1_1 = {};
        VkPhysicalDeviceVulkan12Features m_features_1_2 = {};
        VkPhysicalDeviceVulkan13Features m_features_1_3 = {};

        VkDevice m_device = VK_NULL_HANDLE;

        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        vkb::Instance m_vkbInstance; // vkb::Instance manages VkInstance and debug messenger

        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

#if USE_PHX_ALLOCATOR
        VkAllocationCallbacks m_allocCallbacks = {};
#endif

        // -- VK Queues ---
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        uint32_t m_graphicsQueueFamily = UINT32_MAX;
        std::mutex m_graphicsQueueLock;

        VkQueue m_computeQueue = VK_NULL_HANDLE;
        uint32_t m_computeQueueFamily = UINT32_MAX;
        std::mutex m_computeQueueLock;

        VkQueue m_transferQueue = VK_NULL_HANDLE;
        uint32_t m_transferQueueFamily = UINT32_MAX;
        std::mutex m_transferQueueLock;

        // -- VK Swapchain ---
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_swapchainExtent = { 0, 0 };
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        uint32_t m_swapchainImageIndex = ~0u;

        // -- VK Resources ---
        PagedPool<phx::rhi::GpuBuffer, Buffer_VK> m_bufferPool;

        // -- Frame Data ---
        std::array<FrameData, cMaxInflightFrames> m_frames;
        VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE; // Primary graphics command pool

    };
}