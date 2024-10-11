#pragma once

#include <functional>
#include <deque>

#include "EmberGfx/phxGfxDeviceResources.h"
#include "phxVulkanManager.h"
#include "EmberGfx/phxHandlePool.h"

namespace phx::gfx::platform
{
	constexpr size_t kBufferCount = 2;

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

	struct CommandCtx_Vulkan
	{

	};

	class VulkanGpuDevice final
	{
	public:
		void Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle = nullptr);
		void Finalize();


		void RunGarbageCollection(uint64_t completedFrame = ~0ul);

		CommandCtx_Vulkan* BeingCommandCtx(phx::gfx::CommandQueueType type = CommandQueueType::Graphics);

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

		int32_t RateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

	private:
		bool m_enableValidationLayers;
		VulkanExtManager m_extManager;
		VulkanLayerManager m_layerManager;

		VkInstance m_vkInstance;
		VkPhysicalDevice m_vkPhysicalDevice;
		VkDevice m_vkDevice;

		VkQueue m_vkQueueGfx;
		VkQueue m_vkComputeQueue;
		VkQueue m_vkTransferQueue;

		VkSurfaceKHR m_vkSurface;
		VkDebugUtilsMessengerEXT m_debugMessenger;

		VkSwapchainKHR m_vkSwapChain = VK_NULL_HANDLE;
		VkFormat m_swapChainFormat;
		VkExtent2D m_swapChainExtent;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;


		struct DeferredItem
		{
			uint64_t Frame;
			std::function<void()> DeferredFunc;
		};
		std::deque<DeferredItem> m_deferredQueue;

		VkPipelineCache m_vkPipelineCache;
		HandlePool<PipelineState_Vk, PipelineState> m_pipelineStatePool;
		HandlePool<Shader_VK, Shader> m_shaderPool;
		uint64_t m_frameCount = 0;

		std::vector<VkDynamicState> m_psoDynamicStates;
		VkPipelineDynamicStateCreateInfo m_dynamicStateInfo = {};
		VkPipelineDynamicStateCreateInfo m_dynamicStateInfo_MeshShader = {};

	};
}