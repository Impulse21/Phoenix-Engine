#pragma once

#include <PhxCore/Pool.h>
#include <PhxCore/StaticArray.h>
#include <PhxRhi/PhxRhi_Types.h>

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <deque>

#include "VulkanDescriptorSystem.h"
#include "VulkanSubmissionCtx.h"

#define vulkan_check(call) [&]() { VkResult res = call; PHX_CORE_ASSERT(res >= VK_SUCCESS); return res; }()
#define RHI_DEFINE_ALIGNED(name, alignemnt) alignas(alignemnt) name

namespace phx::rhi
{
    
	constexpr size_t kCacheLineSize = 64ull;
	
	constexpr uint64_t kTimeoutValue = 2000000000ull; // 2 seconds
	constexpr uint32_t kMaxFrameCmds = 64;
	constexpr uint32_t kMaxAsyncCmds = 32;


	struct RHI_DEFINE_ALIGNED(VulkanSwapchainFrame, kCacheLineSize)
	{
		// -- 8-byte members ---
		VkExtent2D			vk_swapchain_extent = { 0, 0 };
		VkSemaphore			vk_image_available_sem;
		VkSemaphore			vk_render_finished_sem;
		VkImage				vk_image;
		VkImageView			vk_image_view;
		VkSwapchainKHR		vk_swapchain = VK_NULL_HANDLE;

		// -- 4-byte members ---
		rhi::ResourceStates resource_state = ResourceStates::Unknown;
		uint32_t			vk_swapchain_image_index = ~0u;

		uint8_t image_index = 0;
	};
	static_assert(sizeof(VulkanSwapchainFrame) == kCacheLineSize, "VulkanSwapchain must be exactly one cache line in size!");
	
	struct VulkanSwapchain
	{
		// -- 8-byte members ---
		std::vector<VkSemaphore>	vk_image_available_sem;
		std::vector<VkSemaphore>	vk_render_finished_sem;

		std::vector<VkImage>		vk_images;
		std::vector<VkImageView>	vk_image_views;

		VkSwapchainKHR				vk_swapchain = VK_NULL_HANDLE;
		VkFormat					vk_swapchain_image_format = VK_FORMAT_UNDEFINED;
	};

	struct RHI_DEFINE_ALIGNED(VulkanBuffer, kCacheLineSize)
	{
		// -- 8-byte members ---
		VkBuffer		vk_buffer;
		VmaAllocation	allocation;

		VkDeviceAddress gpu_address = 0;
		void* mapped_data = nullptr;
		VkBufferView	buffer_view = VK_NULL_HANDLE;

		// -- 4-byte members ---
		uint32_t        mapped_data_size = 0;
#if USE_BUFFER_ADDRESS
		std::byte padding[20];
#else
		DescriptorIndex srv_index = cInvalidDescriptorIndex;
		DescriptorIndex uav_index = cInvalidDescriptorIndex;

		// --- bitfield for booleans (1 byte) ---
		bool            srv_is_typed : 1 = false;
		bool            uav_is_typed : 1 = false;

		// -- Manual Padding ---
		std::byte padding[11];
#endif
	};
	static_assert(sizeof(VulkanBuffer) == kCacheLineSize, "VulkanBuffer must be exactly one cache line in size!");

	struct RHI_DEFINE_ALIGNED(VulkanTexture, kCacheLineSize)
	{
		// -- 8-byte members ---
		VkImage			vk_image;
		VmaAllocation	allocation;

		VkImageView		rtv_image_view = VK_NULL_HANDLE;
		VkImageView		dsv_image_view = VK_NULL_HANDLE;

		// -- 4-byte members ---
		VkImageLayout	default_layout = VK_IMAGE_LAYOUT_GENERAL;
		DescriptorIndex srv_index = cInvalidDescriptorIndex;
		DescriptorIndex uav_index = cInvalidDescriptorIndex;

		// -- Manual Padding ---
		std::byte padding[20];
	};
	static_assert(sizeof(VulkanTexture) == kCacheLineSize, "VulkanTexture must be exactly one cache line in size!");

	struct RHI_DEFINE_ALIGNED(VulkanShaderModule, kCacheLineSize)
	{
		VkShaderModule vk_shader_module = VK_NULL_HANDLE;
	};
	static_assert(sizeof(VulkanShaderModule) == kCacheLineSize, "VulkanShaderModule must be exactly one cache line in size!");

	struct RHI_DEFINE_ALIGNED(VulkanPipelineState, kCacheLineSize)
	{
		VkPipeline                      vk_pipeline;

		VkPipelineBindPoint             bind_point;

		bool                            graphics_pipeline = true;
	};
	static_assert(sizeof(VulkanPipelineState) == kCacheLineSize, "VulkanPipelineState must be exactly one cache line in size!");

    struct DeferredCallbackQueue
	{
		struct DeferredItem
		{
			uint64_t frame;
			std::function<void()> deferred_func;
		};

		std::deque<DeferredItem> queue;

		void Flush(uint64_t completed_frame = ~0u)
		{
			while (!queue.empty())
			{
				DeferredItem& deferred_item = queue.front();
				if (deferred_item.frame + cMaxInflightFrames < completed_frame)
				{
					deferred_item.deferred_func();
					queue.pop_front();
				}
				else
				{
					break;
				}
			}
		}

		void EnqueueDelete(DeferredItem&& item)
		{
			queue.emplace_back(std::forward<DeferredItem>(item));
		}
	};

	struct VulkanQueue
	{
		struct QueueExecutionInfo
		{
			uint64_t fence_value;
			VkCommandBuffer commad_buffer = VK_NULL_HANDLE;
			VkCommandPool command_pool = VK_NULL_HANDLE;
		};

		VkQueue vk_queue = VK_NULL_HANDLE;
		uint32_t vk_queue_family = UINT32_MAX;
		std::vector<std::pair<VkCommandBuffer, VkCommandPool>> pending_commands;
		std::deque<QueueExecutionInfo> available_commands;

		std::mutex lock = {};
		std::mutex lock_commands = {};
	};

	struct VulkanContext
	{
		DeviceCapability capabilities = {};

		// -- VK Core ---
		VkInstance vk_instance = VK_NULL_HANDLE;
		vkb::Instance vkb_instance; // vkb::Instance manages VkInstance and debug messenger

		void* window_handle = nullptr;
		VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

		VkPhysicalDevice vk_chosen_physical_device = VK_NULL_HANDLE;

		VkDevice vk_device = VK_NULL_HANDLE;

		VkPhysicalDeviceFeatures vk_physical_device_features = {};

		VkPhysicalDeviceFeatures2 vk_features2 = {};
		VkPhysicalDeviceVulkan11Features vk_features_1_1 = {};
		VkPhysicalDeviceVulkan12Features vk_features_1_2 = {};
		VkPhysicalDeviceVulkan13Features vk_features_1_3 = {};

		VkPhysicalDeviceProperties2 vk_physical_device_properties = {};
		VkPhysicalDeviceDescriptorBufferPropertiesEXT vk_descriptor_buffer_properties = {};
		VkDeviceSize vk_rebar_heap_size = 0;

		VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;

		VmaAllocator vma_allocator = VK_NULL_HANDLE;
		vulkan::DescriptorSystem descriptor_system;
		vulkan::SubmissionContext submission;

		EnumArray<VulkanQueue, CommandQueueType> queues;
		uint64_t frame_number = 0;

		VkPipelineCache		vk_pipeline_cache = VK_NULL_HANDLE;
		phx::PagedPool<rhi::Swapchain, VulkanSwapchainFrame, VulkanSwapchain> swapchain_pool;
		phx::PagedPool<rhi::Buffer, VulkanBuffer> buffer_pool;
		phx::PagedPool<rhi::PipelineState, VulkanPipelineState> pipeline_state_pool;
		phx::PagedPool<rhi::ShaderModule, VulkanShaderModule> shader_module_pool;

		DeferredCallbackQueue deferred_delete_queue;

	};
    inline VulkanContext g_vulkan;

	void DeleteBufferImmediate(BufferHandle buffer_handle);

	inline CmdHandle EncodeCmdHandle(uint32_t thread_id, CommandQueueType& queue, uint32_t index)
	{
		const uint32_t generation = 0;
		uint32_t q_int = static_cast<uint32_t>(queue);

		// Bit Layout: [Thread:8] [Queue:2] [Gen:8] [Index:14]
		return ((thread_id & 0xFF) << 24) |
			((q_int & 0x03) << 22) |
			((generation & 0xFF) << 14) |
			((index & 0x3FFF) << 0);
	}

	inline void DecodeCmdHande(CmdHandle handle, CommandQueueType& queue, uint32_t& thread_id, uint32_t& index)
	{
		thread_id = (handle >> 24) & 0xFF;
		queue = static_cast<CommandQueueType>((handle >> 22) & 0x03);
		// uint32_t gen		= (handle >> 14) & 0xFF;
		index = (handle >> 0) & 0x3FFF;
	}

	inline VkCommandBuffer ResolveCmdBuffer(CmdHandle handle)
	{
		const uint32_t thread_id	= (handle >> 24) & 0xFF;
		const uint32_t q_int		= (handle >> 22) & 0x03;
		// const uint32_t gen		= (handle >> 14) & 0xFF;
		const uint32_t index		= (handle >> 0) & 0x3FFF;

		// Direct array access - No locks, No maps, No searching.
		vulkan::PerThreadData& thread_data = g_vulkan.submission.per_thread_data[thread_id];
		return thread_data.command_pools[q_int].cmd_buffer_pool[index];
	}

	constexpr VkFormat gVulkanFormatMapping[] =
	{
	   VK_FORMAT_UNDEFINED,                  // UNKNOWN
	   VK_FORMAT_R8_UINT,                    // R8_UINT
	   VK_FORMAT_R8_SINT,                    // R8_SINT
	   VK_FORMAT_R8_UNORM,                   // R8_UNORM
	   VK_FORMAT_R8_SNORM,                   // R8_SNORM
	   VK_FORMAT_R8G8_UINT,                  // RG8_UINT
	   VK_FORMAT_R8G8_SINT,                  // RG8_SINT
	   VK_FORMAT_R8G8_UNORM,                 // RG8_UNORM
	   VK_FORMAT_R8G8_SNORM,                 // RG8_SNORM
	   VK_FORMAT_R16_UINT,                   // R16_UINT
	   VK_FORMAT_R16_SINT,                   // R16_SINT
	   VK_FORMAT_R16_UNORM,                  // R16_UNORM
	   VK_FORMAT_R16_SNORM,                  // R16_SNORM
	   VK_FORMAT_R16_SFLOAT,                 // R16_FLOAT
	   VK_FORMAT_B4G4R4A4_UNORM_PACK16,      // BGRA4_UNORM
	   VK_FORMAT_B5G6R5_UNORM_PACK16,        // B5G6R5_UNORM
	   VK_FORMAT_B5G5R5A1_UNORM_PACK16,      // B5G5R5A1_UNORM
	   VK_FORMAT_R8G8B8A8_UINT,              // RGBA8_UINT
	   VK_FORMAT_R8G8B8A8_SINT,              // RGBA8_SINT
	   VK_FORMAT_R8G8B8A8_UNORM,             // RGBA8_UNORM
	   VK_FORMAT_R8G8B8A8_SNORM,             // RGBA8_SNORM
	   VK_FORMAT_B8G8R8A8_UNORM,             // BGRA8_UNORM
	   VK_FORMAT_R8G8B8A8_SRGB,              // SRGBA8_UNORM
	   VK_FORMAT_B8G8R8A8_SRGB,              // SBGRA8_UNORM
	   VK_FORMAT_A2R10G10B10_UNORM_PACK32,   // R10G10B10A2_UNORM
	   VK_FORMAT_B10G11R11_UFLOAT_PACK32,    // R11G11B10_FLOAT
	   VK_FORMAT_R16G16_UINT,                // RG16_UINT
	   VK_FORMAT_R16G16_SINT,                // RG16_SINT
	   VK_FORMAT_R16G16_UNORM,               // RG16_UNORM
	   VK_FORMAT_R16G16_SNORM,               // RG16_SNORM
	   VK_FORMAT_R16G16_SFLOAT,              // RG16_FLOAT
	   VK_FORMAT_R32_UINT,                   // R32_UINT
	   VK_FORMAT_R32_SINT,                   // R32_SINT
	   VK_FORMAT_R32_SFLOAT,                 // R32_FLOAT
	   VK_FORMAT_R16G16B16A16_UINT,          // RGBA16_UINT
	   VK_FORMAT_R16G16B16A16_SINT,          // RGBA16_SINT
	   VK_FORMAT_R16G16B16A16_SFLOAT,        // RGBA16_FLOAT
	   VK_FORMAT_R16G16B16A16_UNORM,         // RGBA16_UNORM
	   VK_FORMAT_R16G16B16A16_SNORM,         // RGBA16_SNORM
	   VK_FORMAT_R32G32_UINT,                // RG32_UINT
	   VK_FORMAT_R32G32_SINT,                // RG32_SINT
	   VK_FORMAT_R32G32_SFLOAT,              // RG32_FLOAT
	   VK_FORMAT_R32G32B32_UINT,             // RGB32_UINT
	   VK_FORMAT_R32G32B32_SINT,             // RGB32_SINT
	   VK_FORMAT_R32G32B32_SFLOAT,           // RGB32_FLOAT
	   VK_FORMAT_R32G32B32A32_UINT,          // RGBA32_UINT
	   VK_FORMAT_R32G32B32A32_SINT,          // RGBA32_SINT
	   VK_FORMAT_R32G32B32A32_SFLOAT,        // RGBA32_FLOAT

	   VK_FORMAT_D16_UNORM,                  // D16
	   VK_FORMAT_D24_UNORM_S8_UINT,          // D24S8
	   VK_FORMAT_X8_D24_UNORM_PACK32,        // X24G8_UINT
	   VK_FORMAT_D32_SFLOAT,                 // D32
	   VK_FORMAT_D32_SFLOAT_S8_UINT,         // D32S8
	   VK_FORMAT_X8_D24_UNORM_PACK32,        // X32G8_UINT

	   VK_FORMAT_BC1_RGB_UNORM_BLOCK,        // BC1_UNORM
	   VK_FORMAT_BC1_RGB_SRGB_BLOCK,         // BC1_UNORM_SRGB
	   VK_FORMAT_BC2_UNORM_BLOCK,            // BC2_UNORM
	   VK_FORMAT_BC2_SRGB_BLOCK,             // BC2_UNORM_SRGB
	   VK_FORMAT_BC3_UNORM_BLOCK,            // BC3_UNORM
	   VK_FORMAT_BC3_SRGB_BLOCK,             // BC3_UNORM_SRGB
	   VK_FORMAT_BC4_UNORM_BLOCK,            // BC4_UNORM
	   VK_FORMAT_BC4_SNORM_BLOCK,            // BC4_SNORM
	   VK_FORMAT_BC5_UNORM_BLOCK,            // BC5_UNORM
	   VK_FORMAT_BC5_SNORM_BLOCK,            // BC5_SNORM
	   VK_FORMAT_BC6H_UFLOAT_BLOCK,          // BC6H_UFLOAT
	   VK_FORMAT_BC6H_SFLOAT_BLOCK,          // BC6H_SFLOAT
	   VK_FORMAT_BC7_UNORM_BLOCK,            // BC7_UNORM
	   VK_FORMAT_BC7_SRGB_BLOCK,             // BC7_UNORM_SRGB
	};

	static_assert(sizeof(gVulkanFormatMapping) / sizeof(VkFormat) == (int)rhi::Format::COUNT);

	// static assert
	constexpr VkFormat FormatToVkFormat(rhi::Format format)
	{
		return gVulkanFormatMapping[(int)format];
	}

	constexpr std::array<VkShaderStageFlagBits, (size_t)ShaderStage::Count> kShaderStageToVk =
	{
		VK_SHADER_STAGE_MESH_BIT_EXT,                 // MS
		VK_SHADER_STAGE_TASK_BIT_EXT,                 // AS (Amplification == Task in Vulkan)
		VK_SHADER_STAGE_VERTEX_BIT,                   // VS
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,     // HS (Hull == Tess Control)
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,  // DS (Domain == Tess Eval)
		VK_SHADER_STAGE_GEOMETRY_BIT,                 // GS
		VK_SHADER_STAGE_FRAGMENT_BIT,                 // PS (Pixel == Fragment)
		VK_SHADER_STAGE_COMPUTE_BIT,                  // CS
		VK_SHADER_STAGE_ALL                           // LIB (Generic Library)
	};

	// Helper for cleaner casting
	inline VkShaderStageFlagBits ShaderStageToVulkanShaderStage(ShaderStage stage)
	{
		return kShaderStageToVk[static_cast<uint8_t>(stage)];
	}

	constexpr VkComponentSwizzle ComponentSwizzleMap[] =
	{
		VK_COMPONENT_SWIZZLE_R,        // ComponentSwizzle::R
		VK_COMPONENT_SWIZZLE_G,        // ComponentSwizzle::G
		VK_COMPONENT_SWIZZLE_B,        // ComponentSwizzle::B
		VK_COMPONENT_SWIZZLE_A,        // ComponentSwizzle::A
		VK_COMPONENT_SWIZZLE_ZERO,     // ComponentSwizzle::Zero
		VK_COMPONENT_SWIZZLE_ONE,      // ComponentSwizzle::One
	};

	constexpr VkComponentMapping ConvertSwizzle(Swizzle value)
	{
		VkComponentMapping mapping = {};
		mapping.r = ComponentSwizzleMap[(size_t)value.r];
		mapping.g = ComponentSwizzleMap[(size_t)value.g];
		mapping.b = ComponentSwizzleMap[(size_t)value.b];
		mapping.a = ComponentSwizzleMap[(size_t)value.a];
		return mapping;
	}

	constexpr VkBlendFactor ConvertBlendValue(BlendFactor value)
	{
		switch (value)
		{
		case BlendFactor::Zero:
			return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One:
			return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::InvSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::InvDstAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFactor::DstColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::InvDstColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlphaSaturate:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case BlendFactor::ConstantColor:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFactor::InvConstantColor:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case BlendFactor::Src1Color:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case BlendFactor::InvSrc1Color:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case BlendFactor::Src1Alpha:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case BlendFactor::InvSrc1Alpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:
			return VK_BLEND_FACTOR_ZERO;
		}
	}

	constexpr VkColorComponentFlags ToVkColorComponentFlags(ColorMask mask)
	{
		// Cast to integer to allow bitwise operations on enum class
		const uint8_t m = static_cast<uint8_t>(mask);

		VkColorComponentFlags flags = 0;

		if (m & static_cast<uint8_t>(rhi::ColorMask::Red))   flags |= VK_COLOR_COMPONENT_R_BIT;
		if (m & static_cast<uint8_t>(rhi::ColorMask::Green)) flags |= VK_COLOR_COMPONENT_G_BIT;
		if (m & static_cast<uint8_t>(rhi::ColorMask::Blue))  flags |= VK_COLOR_COMPONENT_B_BIT;
		if (m & static_cast<uint8_t>(rhi::ColorMask::Alpha)) flags |= VK_COLOR_COMPONENT_A_BIT;

		return flags;
	}
	constexpr VkBlendOp ConvertBlendOp(EBlendOp value)
	{
		switch (value)
		{
		case EBlendOp::Add:
			return VK_BLEND_OP_ADD;
		case EBlendOp::Subrtact:
			return VK_BLEND_OP_SUBTRACT;
		case EBlendOp::ReverseSubtract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case EBlendOp::Min:
			return VK_BLEND_OP_MIN;
		case EBlendOp::Max:
			return VK_BLEND_OP_MAX;
		default:
			return VK_BLEND_OP_ADD;
		}
	}

	constexpr VkStencilOp ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case StencilOp::Keep:
			return VK_STENCIL_OP_KEEP;
		case StencilOp::Zero:
			return VK_STENCIL_OP_ZERO;
		case StencilOp::Replace:
			return VK_STENCIL_OP_REPLACE;
		case StencilOp::IncrementAndClamp:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOp::DecrementAndClamp:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOp::Invert:
			return VK_STENCIL_OP_INVERT;
		case StencilOp::IncrementAndWrap:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOp::DecrementAndWrap:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}

	constexpr VkCompareOp ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc::Never:
			return VK_COMPARE_OP_NEVER;
		case ComparisonFunc::Less:
			return VK_COMPARE_OP_LESS;
		case ComparisonFunc::Equal:
			return VK_COMPARE_OP_EQUAL;
		case ComparisonFunc::LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case ComparisonFunc::Greater:
			return VK_COMPARE_OP_GREATER;
		case ComparisonFunc::NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;
		case ComparisonFunc::GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case ComparisonFunc::Always:
			return VK_COMPARE_OP_ALWAYS;
		default:
			return VK_COMPARE_OP_NEVER;
		}
	}

	constexpr bool IsFormatDepthSupport(Format format)
	{
		switch (format)
		{
		case Format::D16:
		case Format::D32:
		case Format::D24S8:
			return true;
		default:
			return false;
		}
	}

	constexpr VkImageLayout ResourceStateToImageLayout(ResourceStates value)
	{
		switch (value)
		{
		case ResourceStates::Unknown:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceStates::RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceStates::DepthWrite:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceStates::DepthRead:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ResourceStates::ShaderResource:
		case ResourceStates::ShaderResourceNonPixel:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceStates::UnorderedAccess:
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceStates::CopySource:
		case ResourceStates::CopyDest:
			// Workaround for handling multiple queues with textures in different layouts
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceStates::ShadingRateSurface:
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		case ResourceStates::Present:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		default:
			// Combination of state flags will default to general
			return VK_IMAGE_LAYOUT_GENERAL;
		}
	}
	constexpr VkAccessFlags2 ResourceStateToAccessFlags2(ResourceStates value)
	{
		VkAccessFlags2 flags = VK_ACCESS_2_NONE;

		if (EnumHasAnyFlags(value, ResourceStates::ShaderResource))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::ShaderResourceNonPixel))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::UnorderedAccess))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
			flags |= VK_ACCESS_2_SHADER_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::CopySource))
		{
			flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::CopyDest))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::RenderTarget))
		{
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::DepthWrite))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::DepthRead))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::VertexBuffer))
		{
			flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::IndexGpuBuffer))
		{
			flags |= VK_ACCESS_2_INDEX_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::ConstantBuffer))
		{
			flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::IndirectArgument))
		{
			flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::AccelStructRead))
		{
			flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		}
		if (EnumHasAnyFlags(value, ResourceStates::ShadingRateSurface))
		{
			flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		}
		// Assuming the following states are added to map to the original VIDEO_DECODE_DST and VIDEO_DECODE_SRC
		if (EnumHasAnyFlags(value, ResourceStates::AccelStructWrite))
		{
			flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		}
		if (EnumHasAnyFlags(value, ResourceStates::AccelStructBuildInput))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}

		return flags;
	}

	constexpr VkPipelineStageFlags ResourceStateToPipelineStage(rhi::ResourceStates states)
	{
		VkPipelineStageFlags vk_stages = VK_PIPELINE_STAGE_2_NONE;

		// --- Helper Masks ---
		// Note: Assumes you have KHR extensions enabled in your Vulkan context
		constexpr VkPipelineStageFlags ALL_SHADER_STAGES =
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

		constexpr VkPipelineStageFlags NON_PIXEL_SHADER_STAGES =
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;


			if (EnumHasAnyFlags(states, ResourceStates::ConstantBuffer))
			{
				vk_stages |= ALL_SHADER_STAGES;
			}

			if (EnumHasAnyFlags(states, ResourceStates::VertexBuffer) || EnumHasAnyFlags(states, ResourceStates::IndexGpuBuffer))
			{
				vk_stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::IndirectArgument))
			{
				vk_stages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::ShaderResource))
			{
				vk_stages |= ALL_SHADER_STAGES;
			}

			if (EnumHasAnyFlags(states, ResourceStates::UnorderedAccess))
			{
				vk_stages |= ALL_SHADER_STAGES;
			}

			if (EnumHasAnyFlags(states, ResourceStates::RenderTarget))
			{
				vk_stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::DepthWrite))
			{
				vk_stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::DepthRead))
			{
				// Can be read via tests OR as a shader resource
				vk_stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::StreamOut))
			{
				vk_stages |= VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT; // Requires VK_EXT_transform_feedback
			}

			if (EnumHasAnyFlags(states, ResourceStates::CopyDest) || EnumHasAnyFlags(states, ResourceStates::CopySource) ||
				EnumHasAnyFlags(states, ResourceStates::ResolveDest) || EnumHasAnyFlags(states, ResourceStates::ResolveSource))
			{
				vk_stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::Present))
			{
				// Sync against the final operations before present
				vk_stages |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::AccelStructRead))
			{
				// AS can be read by RT, Compute, or even Vertex shaders
				vk_stages |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			}

			if (EnumHasAnyFlags(states, ResourceStates::AccelStructWrite) || EnumHasAnyFlags(states, ResourceStates::AccelStructBuildInput) || EnumHasAnyFlags(states, ResourceStates::AccelStructBuildBlas))
			{
				vk_stages |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
			}

			if (EnumHasAnyFlags(states, ResourceStates::ShadingRateSurface))
			{
				vk_stages |= VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR; // Requires VK_KHR_fragment_shading_rate
			}

			if (EnumHasAnyFlags(states, ResourceStates::GenericRead))
			{
				// This is a broad "read" state
				vk_stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
					VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
					VK_PIPELINE_STAGE_TRANSFER_BIT |
					ALL_SHADER_STAGES;
			}

			if (EnumHasAnyFlags(states, ResourceStates::ShaderResourceNonPixel))
			{
				vk_stages |= NON_PIXEL_SHADER_STAGES;
			}

			// If no state was specified, default to a safe value
			if (vk_stages == 0)
			{
				vk_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}

			return vk_stages;
	}

	constexpr VkPrimitiveTopology ToVkPrimtivieTopology(rhi::PrimitiveType type)
	{
		switch (type)
		{
		case PrimitiveType::PointList:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::LineList:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::PatchList:
			return  VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		case PrimitiveType::TriangleStrip:
		default:
			return  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
	}
}