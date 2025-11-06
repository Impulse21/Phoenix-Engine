#pragma once

#include <PhxRhi/RHICommon.h>

#include <deque>

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>

#include <vk_mem_alloc.h>

#include <PhxCore/Pool.h>

#include "VkCopyCtxManager.h"
#include "VkBindlessDescriptorArray.h"
#include "VkGpuTempMemory.h"
#include "VkCommandBufferAllocator.h"

#define vulkan_check(call) [&]() { VkResult res = call; PHX_CORE_ASSERT(res >= VK_SUCCESS); return res; }()
#define RHI_DEFINE_ALIGNED(name, alignemnt) alignas(alignemnt) name

namespace phx::rhi
{
	constexpr size_t kMaxNumBuffers = 4096;
	constexpr size_t kMaxNumTextures = 4096;

	constexpr size_t cMaxInflightFrames = 2;
	constexpr uint64_t kTimeoutValue = 2000000000ull; // 2 seconds
	constexpr uint32_t kMaxFrameCmds = 64;
	constexpr uint32_t kMaxAsyncCmds = 32;
	using CommandBufferAllocator = phx::rhi::vk::CommandBufferAllocator_VK<cMaxInflightFrames, kMaxFrameCmds, kMaxAsyncCmds>;


	struct DeferredItem
	{
		uint64_t frame;
		std::function<void()> deferred_func;
	};

	struct VkContext
	{
		inline static bool is_initialized = false;
		inline static bool volk_initialized = false;

		

		inline static rhi::vk::CopyCtxManager copy_ctx_manager = {};



		inline static phx::rhi::vk::VkBindlessDescriptorArray texture_descriptors;

		inline static CommandBufferAllocator command_buffer_allocator;

		// -- Frame Data ---
		inline static std::array<FrameData, cMaxInflightFrames> frames;
		inline static VkCommandPool vk_graphics_command_pool = VK_NULL_HANDLE; // Primary graphics command pool


		inline static std::mutex buffers_mutex;

		static FrameData& GetCurrentFrame() { return frames[frame_number % cMaxInflightFrames]; }


	};


}