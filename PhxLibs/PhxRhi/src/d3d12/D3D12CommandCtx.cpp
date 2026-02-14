#include "PhxRhi_pch.h"
#include "D3D12CommandCtx.h"

#include "D3D12Core.h"
#include "D3D12CommandQueue.h"
#include "D3D12Utils.h"

void phx::rhi::d3d12::D3D12CommandCtx::Reset(rhi::CommandQueueType queueType)
{
	using namespace Microsoft::WRL;

	m_queueType = queueType;
	const D3D12_COMMAND_LIST_TYPE d3d12ListType = ConvertCommandQueueType(m_queueType);
	
	m_allocator = g_commandQueue[m_queueType].RequestAllocator();
	if (m_commandLists[queueType] == nullptr)
	{
		ComPtr<ID3D12GraphicsCommandList> commandList;
		// Create command list
		ThrowIfFailed(
			g_d3d12Device->CreateCommandList(0u, d3d12ListType, m_allocator, nullptr, IID_PPV_ARGS(&commandList)));

		commandList->Close();

		m_commandLists[queueType] = commandList;
	}

	ThrowIfFailed(
		GetGfxCommandList()->Reset(m_allocator, nullptr));

	m_tempAllocator.Reset();

	// Set up bindless heaps
	if (m_queueType == CommandQueueType::Graphics || m_queueType == CommandQueueType::Compute)
	{
		ID3D12DescriptorHeap* heaps[] = {
			g_gpuDescHeap_Resource->GetNativeHeap(),
			g_gpuDescHeap_Sampler->GetNativeHeap()
		};

		GetGfxCommandList()->SetDescriptorHeaps(std::size(heaps), heaps);
	}

	if (m_queueType == CommandQueueType::Graphics)
	{
		D3D12_RECT pRects[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
		for (uint32_t i = 0; i < std::size(pRects); ++i)
		{
			pRects[i].left = 0;
			pRects[i].right = 16384;
			pRects[i].top = 0;
			pRects[i].bottom = 16384;
		}
		GetGfxCommandList()->RSSetScissorRects(std::size(pRects), pRects);
	}
}

void phx::rhi::d3d12::D3D12CommandCtx::EnqueueSubmit()
{
	GetGfxCommandList()->Close();
	g_commandQueue[m_queueType].EnqueueForSubmit(GetCommandList(), GetAllocator());
}
