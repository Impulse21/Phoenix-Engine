#pragma once

#include "D3D12Base.h"
#include "D3D12Core.h"
#include "D3D12Types.h"
#include "D3D12GpuTempMemory.h"

#include <phx/core/Span.h>
#include "phx/core/EnumUtils.h"
#include "phx/rhi/RHITypes.h"
#include <D3D12Utils.h>

namespace phx::rhi::d3d12
{
	class D3D12CommandCtx
	{
	public:
		void Reset(rhi::CommandQueueType queueType);

		inline void RenderPassBegin()
		{
			ID3D12Resource* swapChainImage = g_swapChain.GetBackBuffer();
			D3D12_CPU_DESCRIPTOR_HANDLE view = g_swapChain.GetBackBufferView();

#if false // Enhanced Barriers
			// Define the texture barrier
			D3D12_BARRIER_TEXTURE_BARRIER textureBarrier = {};
			textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
			textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
			textureBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;

			textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_PIXEL_SHADER;
			textureBarrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
			textureBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;

			textureBarrier.pResource = pTexture;  // The texture resource to transition
			textureBarrier.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE(D3D12_BARRIER_SUBRESOURCE_RANGE_FLAG_ALL_SUBRESOURCES);

			// Describe the barrier type
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
			barrierGroup.NumBarriers = 1;
			barrierGroup.pTextureBarriers = &textureBarrier;

#else
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapChainImage;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			GetGfxCommandList()->ResourceBarrier(1, &barrier);
			ClearTexture(view, g_swapChain.ClearColour.Colour);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			m_renderPassBarriers[m_numRenderPasses++] = barrier;
#endif

			GetGfxCommandList()->OMSetRenderTargets(1, &view, 0, nullptr);

		}

		inline void ClearTexture(D3D12_CPU_DESCRIPTOR_HANDLE view, phx::rhi::Color const& clearColour) {
			GetGfxCommandList()->ClearRenderTargetView(
				view,
				&clearColour.R,
				0,
				nullptr);
		}

		inline void RenderPassEnd()
		{
			GetGfxCommandList()->ResourceBarrier((UINT)m_numRenderPasses, m_renderPassBarriers.data());
			m_numRenderPasses = 0;
		}

		inline void SetViewports(phx::Span<Viewport> viewports)
		{
			CD3DX12_VIEWPORT dx12Viewports[16] = {};
			for (size_t i = 0; i < viewports.Size(); i++)
			{
				const Viewport& viewport = viewports[i];
				dx12Viewports[i] = CD3DX12_VIEWPORT(
					viewport.MinX,
					viewport.MinY,
					viewport.GetWidth(),
					viewport.GetHeight(),
					viewport.MinZ,
					viewport.MaxZ);
			}

			GetGfxCommandList()->RSSetViewports((UINT)viewports.Size(), dx12Viewports);
		}

		inline void SetScissors(phx::Span<Rect> scissors) 
		{
			CD3DX12_RECT dx12Scissors[16] = {};
			for (size_t i = 0; i < scissors.Size(); i++)
			{
				const Rect& scissor = scissors[i];

				dx12Scissors[i] = CD3DX12_RECT(
					scissor.MinX,
					scissor.MinY,
					scissor.MaxX,
					scissor.MaxY);
			}

			GetGfxCommandList()->RSSetScissorRects((UINT)scissors.Size(), dx12Scissors);
		
		}
		inline void SetPipelineState(PipelineStateHandle handle) 
		{
			auto pso = g_pipelineStatePool.Get<d3d12::PipelineState>(handle);

			m_activePipelineType = pso->Type;
			GetGfxCommandList()->SetPipelineState(pso->D3D12PipelineState.Get());
			GetGfxCommandList()->SetGraphicsRootSignature(pso->RootSignature.Get());

			GetGfxCommandList()->IASetPrimitiveTopology(pso->Topology);
		}

		inline void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
			GetGfxCommandList()->DrawIndexedInstanced(
				indexCount,
				instanceCount,
				startIndex,
				baseVertex,
				startInstance);
		}

		inline void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) 
		{
			GetGfxCommandList()->DrawInstanced(
				vertexCount,
				instanceCount,
				startVertex,
				startInstance);
		}

		inline void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* vertexBufferData) 
		{
			const size_t vertexSizeInBytes = numVertices * vertexStride;

			TempBuffer tempBuffer = m_tempAllocator.Allocate(static_cast<uint32_t>(vertexSizeInBytes), 4u);
			std::memcpy(tempBuffer.Data, vertexBufferData, vertexSizeInBytes);

			D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
			vertexBufferView.BufferLocation = tempBuffer.GpuAddress;
			vertexBufferView.SizeInBytes = static_cast<UINT>(vertexSizeInBytes);
			vertexBufferView.StrideInBytes = static_cast<UINT>(vertexStride);

			GetGfxCommandList()->IASetVertexBuffers(slot, 1, &vertexBufferView);
		}

		inline void SetDynamicIndexBuffer(size_t numIndicies, Format indexFormat, const void* indexBufferData) 
		{
			const size_t indexStrideInBytes = indexFormat == Format::R16_UINT ? 2 : 4;
			const size_t indexSizeInBytes = numIndicies * indexStrideInBytes;

			TempBuffer tempBuffer = m_tempAllocator.Allocate(static_cast<uint32_t>(indexSizeInBytes), 4u);
			std::memcpy(tempBuffer.Data, indexBufferData, indexStrideInBytes);

			D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
			indexBufferView.BufferLocation = tempBuffer.GpuAddress;
			indexBufferView.SizeInBytes = static_cast<UINT>(indexSizeInBytes);
			const auto& formatMapping = GetDxgiFormatMapping(indexFormat);

			indexBufferView.Format = formatMapping.SrvFormat;
			GetGfxCommandList()->IASetIndexBuffer(&indexBufferView);

		}

		inline void SetPushConstant(uint32_t rootParameterIndex, size_t sizeInBytes, const void* constants) 
		{
			const uint32_t size = static_cast<UINT>(sizeInBytes / sizeof(uint32_t));
			if (this->m_activePipelineType == PipelineState::PipelineType::Compute)
			{
				GetGfxCommandList()->SetComputeRoot32BitConstants(rootParameterIndex, size, constants, 0);
			}
			else
			{
				GetGfxCommandList()->SetGraphicsRoot32BitConstants(rootParameterIndex, size, constants, 0);
			}
		}

	public:
		void EnqueueSubmit();

	public:
		ID3D12CommandAllocator* GetAllocator() { return m_allocator; }
		ID3D12CommandList* GetCommandList() { return m_commandLists[m_queueType].Get(); }
		
		inline ID3D12GraphicsCommandList* GetGfxCommandList()
		{
			return static_cast<ID3D12GraphicsCommandList*>(m_commandLists[m_queueType].Get());
		}

	private:
		CommandQueueType m_queueType;
		ID3D12CommandAllocator* m_allocator;
		EnumArray<Microsoft::WRL::ComPtr<ID3D12CommandList>, CommandQueueType> m_commandLists;

		TempAllocator m_tempAllocator;

		PipelineState::PipelineType m_activePipelineType = PipelineState::PipelineType::Gfx;
		std::array<D3D12_RESOURCE_BARRIER, rhi::cMaxRenderTargets> m_renderPassBarriers;
		size_t m_numRenderPasses = 0;
		
	};

}
