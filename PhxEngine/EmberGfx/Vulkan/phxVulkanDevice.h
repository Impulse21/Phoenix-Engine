#pragma once

#include <functional>
#include <deque>
#include <mutex>

#include "phxEnumUtils.h"

#include "EmberGfx/phxGfxDeviceResources.h"
#include "EmberGfx/phxHandlePool.h"

#include "phxVulkanManager.h"
#include "phxVulkanCommandCtx.h"

#include "vma/vk_mem_alloc.h"

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

namespace phx::gfx::platform
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> ComputeFamily;
		std::optional<uint32_t> TransferFamily;

		bool IsComplete() 
		{
			return 
				GraphicsFamily.has_value() &&
				TransferFamily.has_value() &&
				ComputeFamily.has_value();
		}
	};

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	struct PipelineState_Vk
	{
		VkPipeline                      Pipeline;
		VkPipelineLayout                PipelineLayout;

		VkPipelineBindPoint             BindPoint;

		// ShaderStateHandle               shader_state;

		// const DesciptorSetLayout* descriptor_set_layout[k_max_descriptor_set_layouts];
		// DescriptorSetLayoutHandle       descriptor_set_layout_handle[k_max_descriptor_set_layouts];
		// u32                             num_active_layouts = 0;

		// DepthStencilCreation            depth_stencil;
		// BlendStateCreation              blend_state;
		// RasterizationCreation           rasterization;
		bool                            graphics_pipeline = true;
	};

	struct Shader_VK
	{
		VkShaderModule ShaderModule = VK_NULL_HANDLE;
		// VkPipeline PipelineCS = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo StageInfo = {};
		// VkPipelineLayout pipelineLayout_cs = VK_NULL_HANDLE; // no lifetime management here
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE; // no lifetime management here
		std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
		VkPushConstantRange Pushconstants = {};
		std::vector<uint32_t> UniformBufferDynamicSlots;
		size_t BindingHash = 0;
	};

	class VulkanGpuDevice final
	{
		friend CommandCtx_Vulkan;
	public:
		void Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle = nullptr);
		void Finalize();


		void RunGarbageCollection(uint64_t completedFrame = ~0ul);

		CommandCtx_Vulkan* BeginCommandCtx(phx::gfx::CommandQueueType type = CommandQueueType::Graphics);
		void SubmitFrame();

		// Resource Factory
	public:
		ShaderHandle CreateShader(ShaderDesc const& desc);
		void DeleteShader(ShaderHandle handle);

		PipelineStateHandle CreatePipeline(PipelineStateDesc2 const& desc);
		void DeletePipeline(PipelineStateHandle handle);

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSurface(void* windowHandle);
		void CreateSwapchain(SwapChainDesc const& desc);
		void CreateSwapChaimImageViews();
		void CreateVma();
		void CreateFrameResources();
		void CreateDefaultResources();

		void DestoryFrameResources();
		void DestoryDefaultResources();

		uint32_t GetBufferIndex() const { return m_frameCount % kBufferCount; }

		int32_t RateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

		void SubmitCommandCtx();
		void Present();

	private:
		bool m_enableValidationLayers;
		VulkanExtManager m_extManager;
		VulkanLayerManager m_layerManager;

		GpuDeviceCapabilities m_capabilities = {};
		VkInstance m_vkInstance;
		VkPhysicalDevice m_vkPhysicalDevice;
		VkDevice m_vkDevice;

		VmaAllocator m_vmaAllocator = {};

		VkPhysicalDeviceFeatures2 m_features2 = {};
		VkPhysicalDeviceVulkan12Features m_vulkan12Features = {};
		VkPhysicalDeviceVulkan13Features m_vulkan13Features = {};
		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT m_extendedDynamicStateFeatures = {};
		VkPhysicalDeviceMemoryProperties2 m_memoryProperties2 = {};

		QueueFamilyIndices m_queueFamilies;
		struct CommandQueue
		{
			VkQueue QueueVk = VK_NULL_HANDLE;

			std::vector<VkSemaphoreSubmitInfo> SubmitWaitSemaphoreInfos;
			std::vector<VkSemaphore> SubmitSignalSemaphores;
			std::vector<VkSemaphoreSubmitInfo> SubmitSignalSemaphoreInfos;
			std::vector<VkCommandBufferSubmitInfo> SubmitCmds;

			std::mutex m_mutex;

			void Signal(VkSemaphore semaphore);
			void Wait(VkSemaphore semaphore);
			void Submit(VulkanGpuDevice* device, VkFence fence);
		};

		EnumArray<CommandQueue, CommandQueueType> m_queues;

		VkSurfaceKHR m_vkSurface;
		VkDebugUtilsMessengerEXT m_debugMessenger;

		SwapChainDesc m_swapChainDesc = {};
		VkSwapchainKHR m_vkSwapChain = VK_NULL_HANDLE;
		uint32_t m_swapChainCurrentImage = ~0u;

		VkFormat m_swapChainFormat;
		VkExtent2D m_swapChainExtent;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;

		std::array<VkSemaphore, kBufferCount> m_imageAvailableSemaphore;
		std::array< VkSemaphore, kBufferCount> m_renderFinishedSemaphore;


		struct DeferredItem
		{
			uint64_t Frame;
			std::function<void()> DeferredFunc;
		};
		std::deque<DeferredItem> m_deferredQueue;

		VkPipelineCache m_vkPipelineCache;
		HandlePool<PipelineState_Vk, PipelineState> m_pipelineStatePool;
		HandlePool<Shader_VK, Shader> m_shaderPool;


		std::mutex m_commandPoolLock;
		std::vector<std::unique_ptr<CommandCtx_Vulkan>> m_commandCtxPool;
		uint32_t m_commandCtxCount;
		uint64_t m_frameCount = 0;
		std::array<EnumArray<VkFence, CommandQueueType>, kBufferCount> m_frameFences;

		std::vector<VkDynamicState> m_psoDynamicStates;
		VkPipelineDynamicStateCreateInfo m_dynamicStateInfo = {};
		VkPipelineDynamicStateCreateInfo m_dynamicStateInfo_MeshShader = {};

		VkBuffer m_nullBuffer = VK_NULL_HANDLE;
		VmaAllocation m_nullBufferAllocation = VK_NULL_HANDLE;
		VkBufferView m_nullBufferView = VK_NULL_HANDLE;

	};
}