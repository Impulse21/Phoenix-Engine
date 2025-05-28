#include "PhxRhi/PhxRhi_pch.h"

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <sstream>
#include <PhxCore/CommandLineArgs.h>

#include "PhxRhi/RHICommandCtx.h"
#include "PhxRhi/RHITypes.h"
#include "PhxRhi/RHICore.h"


#include "VkCore.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define VOLK_IMPLEMENTATION
#include "volk.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#define SAFE_DELETE(x) if (x) { delete x; }

#ifdef PHX_PLATFORM_WINDOWS
extern HINSTANCE g_hInstance;
#endif

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vk;

namespace
{
	const GUID kRenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID kPixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

}

namespace phx::rhi::vk
{
	VkContext g_VkContext;
	size_t g_frameCount = 0;
}

namespace
{
	void RunGarbageCollection(uint64_t completedFrame)
	{
	}
}

namespace phx::rhi
{
	void Initialize(RhiCreateInfo const& createInfo)
	{
		PHX_PROFILE_SECTION("Vulkan::Initialize");
		PHX_CORE_INFO("Initializing RHI(Vulkan)");

		volkInitialize();

#if PHX_DEBUG
		// Default to true for debug builds
		bool bUseValidationLayers = false;
#else
		bool bUseValidationLayers = false;
#endif

		vkb::InstanceBuilder builder;
		//make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name("Vulkan Application")
			.set_engine_name("Phx Engine")
			.request_validation_layers(bUseValidationLayers)
			.use_default_debug_messenger()
			.build();

		vkb::Instance vkbInstance = inst_ret.value();
		g_VkContext.Instance = vkbInstance.instance;
		volkLoadInstance(g_VkContext.Instance);
		PHX_CORE_INFO("[RHI] Vulkan Instance initialized {0}", vkbInstance.api_version);

#ifdef PHX_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfow = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // structType
			nullptr, // pNext
			0, // flags
			g_hInstance, // hinstance
			(HWND)createInfo.WindowsHandle};


		VkResult result = 
			vkCreateWin32SurfaceKHR(
				g_VkContext.Instance,
				&surfaceCreateInfow,
				nullptr,
				&g_VkContext.Surface);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
#else
		static_assert(false, "Not implemented platform");
#endif

		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.2
		vkb::PhysicalDeviceSelector selector{ vkbInstance };
		VkPhysicalDeviceFeatures feats{};

		feats.pipelineStatisticsQuery = true;
		feats.multiDrawIndirect = true;
		feats.drawIndirectFirstInstance = true;
		feats.samplerAnisotropy = true;
		feats.sparseBinding = true;
		selector.set_required_features(feats);

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

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.require_separate_compute_queue()
			.require_separate_transfer_queue()
			.set_surface(g_VkContext.Surface)
			.add_required_extensions(required_extensions.size(), required_extensions.data())
			.select()
			.value();

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

		physicalDevice.enable_extensions_if_present(optional_extensions);

		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		g_VkContext.Device = vkbDevice.device;
		volkLoadDevice(g_VkContext.Device);

		g_VkContext.ChoosenPhysicalDevice = physicalDevice.physical_device;
		g_VkContext.PhysicalDeviceProperties = physicalDevice.properties;

		// use vkbootstrap to get a Graphics queue
		g_VkContext.GfxQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		g_VkContext.GfxQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		g_VkContext.ComputeQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		g_VkContext.ComputeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		g_VkContext.TransferQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
		g_VkContext.TransferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = g_VkContext.ChoosenPhysicalDevice;
		allocatorInfo.device = g_VkContext.Device;
		allocatorInfo.instance = g_VkContext.Instance;

		// Core in 1.1
		allocatorInfo.flags =
			VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
			VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

	
		if (vkbDevice.physical_device.is_extension_present(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
		{
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}

#if VMA_DYNAMIC_VULKAN_FUNCTIONS
		static VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
#endif

		std::vector<VkExternalMemoryHandleTypeFlags> externalMemoryHandleTypes;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		externalMemoryHandleTypes.resize(physicalDevice.memory_properties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#elif defined(__linux__)
		externalMemoryHandleTypes.resize(physicalDevice.memory_properties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
		allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#endif

		VkResult res = vmaCreateAllocator(&allocatorInfo, &g_VkContext.VmaAllocator);
		PHX_CORE_ASSERT(res == VK_SUCCESS);
		if (res != VK_SUCCESS)
		{
			PHX_CORE_ERROR("Failed to create VMA allocator");
			throw std::runtime_error("vmaCreateAllocator failed!");
		}

		VkPhysicalDeviceMemoryProperties memProperties = physicalDevice.memory_properties;

		std::stringstream memory_ss;
		for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i) 
		{
			const auto& heap = memProperties.memoryHeaps[i];
			memory_ss << "\t\t\tHeap " << i
				<< ": Size = " << PhxToMB(heap.size) << " MB, "
				<< ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "Device Local" : "System Shared")
				<< std::endl;
		}

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) 
		{
			const auto& type = memProperties.memoryTypes[i];
			memory_ss << "\t\t\tType " << i
				<< ": Heap = " << type.heapIndex
				<< ", Flags = " << type.propertyFlags
				<< std::endl;
		}

		std::string memoryInfo = memory_ss.str();
		PHX_CORE_INFO(
			"Physical Device Chosen:\t {0}\n\t\t Min Buffer Alignment: {1} \n\t\t Memory Details:\n{2}",
			g_VkContext.PhysicalDeviceProperties.deviceName,
			g_VkContext.PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
			memoryInfo.c_str());
	}

	void Finalize()
	{
		WaitForIdle();

	}


	Budget GetBudget()
	{
		return {};
	}

	void WaitForIdle()
	{
	}

	CommandCtx* BeginCommnadCtx(CommandQueueType queueType)
	{
		return nullptr;
	}

	void Present()
	{
		RunGarbageCollection(g_frameCount);
	}

	ShaderFormat GetShaderFormat() 
	{ 
		return ShaderFormat::Spriv;
	}
}