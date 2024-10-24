#include "pch.h"

#include "phxVulkanDevice.h"
#include "phxVulkanCommandCtx.h"

void phx::gfx::platform::CommandCtx_Vulkan::RenderPassBegin()
{
	BarriersRenderPassEnd.clear();

	VkResult res = vkAcquireNextImageKHR(
		GpuDevice->m_vkDevice,
		GpuDevice->m_vkSwapChain,
		UINT64_MAX,
		GpuDevice->m_imageAvailableSemaphore[GpuDevice->GetBufferIndex()],
		VK_NULL_HANDLE,
		&GpuDevice->m_swapChainCurrentImage
	);

	if (res != VK_SUCCESS)
	{
		// Handle outdated error in acquire:
		if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			GpuDevice->RecreateSwapchain();
			RenderPassBegin();
			return;
		}
		assert(0);
	}


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

void phx::gfx::platform::CommandCtx_Vulkan::SetPipelineState(PipelineStateHandle pipelineState)
{
	PipelineState_Vk* impl = GpuDevice->m_pipelineStatePool.Get(pipelineState);
	assert(impl);

	vkCmdBindPipeline(GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, impl->Pipeline);
}

void phx::gfx::platform::CommandCtx_Vulkan::SetViewports(Span<Viewport> viewports)
{
	VkViewport vp[16];
	for (size_t i = 0; i < viewports.Size(); i++)
	{
		vp[i].x = viewports[i].MinX;
		vp[i].y = viewports[i].MinY + viewports[i].MaxY;
		vp[i].width = viewports[i].GetWidth();
		vp[i].height = -viewports[i].GetHeight();
		vp[i].minDepth = viewports[i].MinZ;
		vp[i].maxDepth = viewports[i].MaxZ;
	}

	vkCmdSetViewportWithCount(GetVkCommandBuffer(), viewports.Size(), vp);
}

void phx::gfx::platform::CommandCtx_Vulkan::SetScissors(Span<Rect> rects)
{
	VkRect2D scissors[16];
	for (size_t i = 0; i < rects.Size(); i++)
	{
		scissors[i].extent.width = rects[i].GetWidth();
		scissors[i].extent.height = rects[i].GetHeight();
		scissors[i].offset.x = std::max(0, rects[i].MinX);
		scissors[i].offset.y = std::max(0, rects[i].MinY);
	}

	vkCmdSetScissorWithCount(GetVkCommandBuffer(), rects.Size(), scissors);
}

void phx::gfx::platform::CommandCtx_Vulkan::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	vkCmdDrawIndexed(GetVkCommandBuffer(), indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void phx::gfx::platform::CommandCtx_Vulkan::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	vkCmdDraw(GetVkCommandBuffer(), vertexCount, instanceCount, startVertex, startInstance);
}

void phx::gfx::platform::CommandCtx_Vulkan::SetDynamicVertexBuffer(BufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
{
	const VkDeviceSize bufferSize = numVertices * vertexSize;
	const VkDeviceSize vertexStride = vertexSize;

	const Buffer_VK* bufferImpl = GpuDevice->m_bufferPool.Get(tempBuffer);
#if true
	vkCmdBindVertexBuffers2(GetVkCommandBuffer(), 0, 1, &bufferImpl->BufferVk, &offset, &bufferSize, &vertexStride);
#else
	vkCmdBindVertexBuffers(GetVkCommandBuffer(), 0, 1, &bufferImpl->BufferVk, &offset);
#endif
}

void phx::gfx::platform::CommandCtx_Vulkan::SetDynamicIndexBuffer(BufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
{
	// TODO: Format mapping

	VkIndexType type = indexFormat == Format::R16_UINT
		? VK_INDEX_TYPE_UINT16
		: VK_INDEX_TYPE_UINT32;

	const Buffer_VK* bufferImpl = GpuDevice->m_bufferPool.Get(tempBuffer);
	vkCmdBindIndexBuffer(GetVkCommandBuffer(), bufferImpl->BufferVk, offset, type);
}

void phx::gfx::platform::CommandCtx_Vulkan::SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
{
}
