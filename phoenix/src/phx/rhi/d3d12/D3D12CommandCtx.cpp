#include "phxpch.h"
#include "D3D12CommandCtx.h"

#include "D3D12Core.h"
#include "D3D12CommandQueue.h"
#include "D3D12Utils.h"

void phx::rhi::d3d12::D3D12CommandCtx::Reset(rhi::CommandQueueType queueType)
{
	using namespace Microsoft::WRL;

	m_queueType = queueType;
	const D3D12_COMMAND_LIST_TYPE d3d12ListType = ConvertCommandQueueType(m_queueType);

	// request an allocator

	m_allocator = g_commandQueue[m_queueType].RequestAllocator();
	if (m_commandLists[queueType] == nullptr)
	{
		// Create command list
		ThrowIfFailed(
			g_d3d12Device->CreateCommandList(0u, d3d12ListType, m_allocator, nullptr, IID_PPV_ARGS(&m_commandLists[m_queueType])));
	}
}
