#pragma once

#include "D3D12/phxGfxPlatform.h"

namespace phx::gfx
{
	class CommandCtx
	{
	public:
		CommandCtx(PlatformCommandCtx* platform)
			: m_platform(platform)
		{};


		void TransitionBarrier(GpuBarrier const& barrier)
		{
			this->m_platform->TransitionBarrier(barrier);
		}

		void TransitionBarriers(Span<GpuBarrier> gpuBarriers)
		{
			this->m_platform->TransitionBarriers(gpuBarriers);
		}

		void ClearBackBuffer(Color const& clearColour)
		{
			this->m_platform->ClearBackBuffer(clearColour);
		}

		void ClearTextureFloat(TextureHandle texture, Color const& clearColour)
		{
			this->m_platform->ClearTextureFloat(texture, clearColour);
		}

		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil)
		{
			this->m_platform->ClearDepthStencilTexture(depthStencil, clearDepth, depth, clearStencil, stencil);
		}

		void SetGfxPipeline(GfxPipelineHandle pipeline)
		{
			this->m_platform->SetGfxPipeline(pipeline);
		}

		void SetRenderTargetSwapChain()
		{
			this->m_platform->SetRenderTargetSwapChain();
		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
		{
			this->m_platform->Draw(vertexCount, instanceCount, startVertex, startInstance);
		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
		{
			this->m_platform->DrawIndexed(indexCount, instanceCount, startIndex, baseVertex, startInstance);
		}

		void SetViewports(Span<Viewport> viewports)
		{
			this->m_platform->SetViewports(viewports);
		}

		void WriteTexture(TextureHandle texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData)
		{
			this->m_platform->WriteTexture(texture, firstSubresource, numSubresources, pSubresourceData);
		}

		void SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthStencil)
		{
			this->m_platform->SetRenderTargets(renderTargets, depthStencil);
		}

		
		void SetDynamicVertexBuffer(BufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
		{
			this->m_platform->SetDynamicVertexBuffer(tempBuffer, offset, slot, numVertices, vertexSize);
		}

		void SetIndexBuffer(BufferHandle indexBuffer)
		{
			this->m_platform->SetIndexBuffer(indexBuffer);
		}

		void SetDynamicIndexBuffer(BufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
		{
			this->m_platform->SetDynamicIndexBuffer(tempBuffer, offset, numIndicies, indexFormat);
		}

		void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
		{
			this->m_platform->SetPushConstant(rootParameterIndex, sizeInBytes, constants);
		}

		void SetScissors(Span<Rect> scissors)
		{
			this->m_platform->SetScissors(scissors);
		}

		DynamicBuffer AllocateDynamic(size_t sizeInBytes, size_t alignment = 16)
		{
			return this->m_platform->AllocateDynamic(sizeInBytes, alignment);
		}

	private:
		PlatformCommandCtx* m_platform;
	};

}