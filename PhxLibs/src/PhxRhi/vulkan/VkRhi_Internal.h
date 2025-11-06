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
	using CommandBufferAllocator = phx::rhi::vk::CommandBufferAllocator_VK<cMaxInflightFrames, kMaxFrameCmds, kMaxAsyncCmds>;




}