#include "pch.h"

#include "phxVulkanDevice.h"
#include "phxVulkanCommandCtx.h"

void phx::gfx::platform::CommandCtx_Vulkan::RenderPassBegin()
{
	BarriersRenderPassEnd.clear();

	VkRenderingInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	info.renderArea.offset.x = 0;
	info.renderArea.offset.y = 0;
	info.renderArea.extent.width = GpuDevice->m_swapChainExtent.width;
	info.renderArea.extent.height = GpuDevice->m_swapChainExtent.height;
	info.layerCount = 1;

	const uint32_t currentImage = GpuDevice->m_swapChainCurrentImage;
	VkRenderingAttachmentInfo colourAttachment = {};
	colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colourAttachment.imageView = GpuDevice->m_swapChainImageViews[currentImage];
	colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.clearValue.color.float32[0] = GpuDevice->m_swapChainDesc.OptmizedClearValue.Colour.R;
	colourAttachment.clearValue.color.float32[1] = GpuDevice->m_swapChainDesc.OptmizedClearValue.Colour.G;
	colourAttachment.clearValue.color.float32[2] = GpuDevice->m_swapChainDesc.OptmizedClearValue.Colour.B;
	colourAttachment.clearValue.color.float32[3] = GpuDevice->m_swapChainDesc.OptmizedClearValue.Colour.A;

	info.colorAttachmentCount = 1;
	info.pColorAttachments = &colourAttachment;

	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = GpuDevice->m_swapChainImages[currentImage];
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_NONE;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	VkDependencyInfo dependencyInfo = {};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(GetVkCommandBuffer(), &dependencyInfo);

	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_NONE;
	BarriersRenderPassEnd.push_back(barrier);

	vkCmdBeginRendering(GetVkCommandBuffer(), &info);
}

void phx::gfx::platform::CommandCtx_Vulkan::RenderPassEnd()
{
	vkCmdEndRendering(GetVkCommandBuffer());
	if (!BarriersRenderPassEnd.empty())
	{
		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(BarriersRenderPassEnd.size());
		dependencyInfo.pImageMemoryBarriers = BarriersRenderPassEnd.data();

		vkCmdPipelineBarrier2(GetVkCommandBuffer(), &dependencyInfo);
	}
}