#include "PhxRhi/PhxRhi_pch.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxCore/Math.h>

#include "VkRhi_Internal.h"

#include "VkCopyCtxManager.h"

using namespace phx::RHI::vk;

void CopyCtxManager::Initialize()
{
}

void CopyCtxManager::Shutdown()
{
	vkQueueWaitIdle(VkContext::vk_transfer_queue);
	for (auto& x : free_list)
	{
		VkDevice vk_logical_device = VkContext::vk_device;
		vkDestroyCommandPool(vk_logical_device, x.transfer_command_pool, nullptr);
		vkDestroyCommandPool(vk_logical_device, x.transition_command_pool, nullptr);
		vkDestroySemaphore(vk_logical_device, x.semaphore, nullptr);
		vkDestroyFence(vk_logical_device, x.fence, nullptr);

		RHI::DeleteBuffer(x.upload_buffer);
	}
}

CopyCtx CopyCtxManager::Allocate(uint64_t staging_size)
{
	CopyCtx copy_ctx;
	{
		std::scoped_lock _(lock);
		for (size_t i = 0; i < free_list.size(); i++)
		{
			Buffer_VK* free_buffer = VkContext::buffer_pool.GetHot(free_list[i].upload_buffer);
			if (free_buffer->mapped_data_size >= staging_size)
			{
				copy_ctx = std::move(free_list[i]);
				std::swap(free_list[i], free_list.back());
				free_list.pop_back();
				break;
			}
		}
	}

	VkDevice vk_logical_device = VkContext::vk_device;
	if (!copy_ctx.IsValid())
	{
		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		pool_info.queueFamilyIndex = VkContext::vk_transfer_queue_family;
		vulkan_check(
			vkCreateCommandPool(vk_logical_device, &pool_info, nullptr, &copy_ctx.transfer_command_pool));

		pool_info.queueFamilyIndex = VkContext::vk_graphics_queue_family;
		vulkan_check(
			vkCreateCommandPool(vk_logical_device, &pool_info, nullptr, &copy_ctx.transition_command_pool));

		VkCommandBufferAllocateInfo command_buffer_info = {};
		command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_info.commandBufferCount = 1;
		command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_info.commandPool = copy_ctx.transfer_command_pool;

		vulkan_check(
			vkAllocateCommandBuffers(vk_logical_device, &command_buffer_info, &copy_ctx.transfer_command_buffer));

		command_buffer_info.commandPool = copy_ctx.transition_command_pool;
		vulkan_check(
			vkAllocateCommandBuffers(vk_logical_device, &command_buffer_info, &copy_ctx.transition_command_buffer));

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vulkan_check(
			vkCreateFence(vk_logical_device, &fenceInfo, nullptr, &copy_ctx.fence));

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vulkan_check(
			vkCreateSemaphore(vk_logical_device, &semaphoreInfo, nullptr, &copy_ctx.semaphore));

		copy_ctx.upload_buffer = RHI::CreateBuffer({
				.Size = static_cast<uint32_t>(std::max(phx::math::GetNextPowerOfTwo(staging_size), uint64_t(65536))),
				.Usage = RHI::Usage::Upload,
			});
	}

	// begin command list in valid state:
	vulkan_check(
		vkResetCommandPool(vk_logical_device, copy_ctx.transfer_command_pool, 0));
	vulkan_check(
		vkResetCommandPool(vk_logical_device, copy_ctx.transition_command_pool, 0));

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vulkan_check(
		vkBeginCommandBuffer(copy_ctx.transfer_command_buffer, &beginInfo));

	vulkan_check(
		vkBeginCommandBuffer(copy_ctx.transition_command_buffer, &beginInfo));

	vulkan_check(
		vkResetFences(vk_logical_device, 1, &copy_ctx.fence));

	return copy_ctx;
}

void phx::RHI::vk::CopyCtxManager::SubmitAndWait(CopyCtx copy_ctx)
{
	vulkan_check(
		vkEndCommandBuffer(copy_ctx.transfer_command_buffer));
	vulkan_check(
		vkEndCommandBuffer(copy_ctx.transition_command_buffer));

	VkSubmitInfo2 submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

	VkCommandBufferSubmitInfo cb_submit_info = {};
	cb_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

	VkSemaphoreSubmitInfo signal_semaphore_info = {};
	signal_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

	VkSemaphoreSubmitInfo wait_semaphore_info = {};
	wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

	{
		cb_submit_info.commandBuffer = copy_ctx.transfer_command_buffer;
		signal_semaphore_info.semaphore = copy_ctx.semaphore; // signal for graphics queue
		signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &cb_submit_info;
		submit_info.signalSemaphoreInfoCount = 1;
		submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;

		std::scoped_lock lock(VkContext::transfer_queue_lock);
		vulkan_check(
			vkQueueSubmit2(VkContext::vk_transfer_queue, 1, &submit_info, VK_NULL_HANDLE));
	}

	{
		wait_semaphore_info.semaphore = copy_ctx.semaphore; // wait for init queue
		wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		cb_submit_info.commandBuffer = copy_ctx.transition_command_buffer;

		submit_info.waitSemaphoreInfoCount = 1;
		submit_info.pWaitSemaphoreInfos = &wait_semaphore_info;
		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &cb_submit_info;
		submit_info.signalSemaphoreInfoCount = 0;
		submit_info.pSignalSemaphoreInfos = nullptr;

		std::scoped_lock lock(VkContext::graphics_queue_lock);
		vulkan_check(
			vkQueueSubmit2(VkContext::vk_graphics_queue, 1, &submit_info, copy_ctx.fence));
	}

	while (vulkan_check(vkWaitForFences(RHI::VkContext::vk_device, 1, &copy_ctx.fence, VK_TRUE, kTimeoutValue)) == VK_TIMEOUT)
	{
		PHX_CORE_ERROR("[Vulkan] Copy Ctx Manager vkWaitForFences resulted in VK_TIMEOUT");
		std::this_thread::yield();
	}

	std::scoped_lock _(lock);
	free_list.push_back(copy_ctx);
}


