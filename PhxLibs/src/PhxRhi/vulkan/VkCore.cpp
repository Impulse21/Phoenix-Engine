#include "PhxRhi/PhxRhi_pch.h"

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <PhxCore/CommandLineArgs.h>

#include "PhxRhi/RHICommandCtx.h"
#include "PhxRhi/RHITypes.h"
#include "PhxRhi/RHICore.h"


#include "VkCore.h"


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

		PHX_CORE_INFO("[RHI] Vulkan Instance initialized {0}", vkbInstance.api_version);

#ifdef PHX_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfow = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // structType
			nullptr, // pNext
			0, // flags
			g_hInstance, // hinstance
			(HWND)createInfo.WindowsHandle};

		VkSurfaceKHR surface;

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

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.require_separate_compute_queue()
			.require_separate_transfer_queue()
			.set_surface(g_VkContext.Surface)
			.add_required_extension(VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME)

			.select()
			.value();
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