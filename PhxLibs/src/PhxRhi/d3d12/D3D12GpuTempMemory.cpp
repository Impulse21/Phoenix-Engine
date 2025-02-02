#include "PhxRhi/PhxRhi_pch.h"

#include "D3D12GpuTempMemory.h"

#include "D3D12Core.h"
#include "D3D12CommandQueue.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;

TempMemoryBlockAllocator::TempMemoryBlock TempMemoryBlockAllocator::GetNextMemoryBlock()
{
	std::scoped_lock _(this->m_mutex);

	// Checks if the top bits have changes, if so, we need to wrap around.
	if ((m_tail ^ (m_tail + m_blockSize)) & ~m_bufferMask)
	{
		m_tail = (m_tail + m_bufferMask) & ~m_bufferMask;
	}

	if (((m_tail - m_head) + m_blockSize) >= GetBufferSize())
	{
		WaitForFreeRegions(m_head);
	}

	const uint32_t offset = (this->m_tail & m_bufferMask) + m_blockSize;
	m_tail += m_blockSize;

	return TempMemoryBlock{
		.GpuAddress = m_buffer->GetGPUVirtualAddress() + offset,
		.Data = static_cast<uint8_t*>(this->m_data) + offset,
	};
}

void TempMemoryBlockAllocator::Initialize(uint32_t bufferSize, uint32_t blockSize)
{
	PHX_ASSERT((bufferSize & (bufferSize - 1)) == 0, "Buffer Size must be a power of 2");

	m_blockSize = blockSize;
	m_bufferMask = (bufferSize - 1);

	// TODO: Create Buffer

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
#if true
	allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
#else
	allocationDesc.HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
#endif

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resourceFlags, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	ThrowIfFailed(
		g_d3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			initialState,
			nullptr,
			&m_allocation,
			IID_PPV_ARGS(&m_buffer)));


	D3D12_RANGE readRange = {};
	ThrowIfFailed(
		m_buffer->Map(0, &readRange, &m_data));
}

void TempMemoryBlockAllocator::Finalize()
{
	while (!this->m_inUseRegions.empty())
	{
		auto& region = this->m_inUseRegions.front();
		if (region.Fence->GetCompletedValue() != 1)
		{
			region.Fence->SetEventOnCompletion(1, nullptr);
		}

		this->m_inUseRegions.pop_front();
	}

	// TODO: Determine if I need to unmap??
}

void TempMemoryBlockAllocator::EndFrame()
{
	const uint32_t usedSize = m_tail - m_headAtStartOfFrame;
	while (!this->m_inUseRegions.empty())
	{
		auto& region = this->m_inUseRegions.front();
		if (region.Fence->GetCompletedValue() != 1)
		{
			break;
		}

		region.Fence->Signal(0);
		this->m_availableFences.push_back(region.Fence);

		m_head += region.UsedSize;

		this->m_inUseRegions.pop_front();
	}

	ID3D12Fence* fence = nullptr;
	if (!this->m_availableFences.empty())
	{
		fence = this->m_availableFences.front();
		this->m_availableFences.pop_front();
	}

	if (!fence)
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> newFence;
		g_d3d12Device2->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(newFence.ReleaseAndGetAddressOf()));
		this->m_fencePool.push_back(newFence);
		fence = newFence.Get();
	}

	g_commandQueue[rhi::CommandQueueType::Graphics].Queue->Signal(fence, 1);
	this->m_inUseRegions.push_front(UsedRegion{
		.UsedSize = usedSize,
		.Fence = fence });
	m_headAtStartOfFrame = m_tail;
}

void TempMemoryBlockAllocator::WaitForFreeRegions(uint32_t& head)
{
	while (!this->m_inUseRegions.empty())
	{
		auto& region = this->m_inUseRegions.front();
		if (region.Fence->GetCompletedValue() != 1)
		{
			PHX_CORE_WARN("[GPU QUEUE] Stalling waiting for space");
			region.Fence->SetEventOnCompletion(1, nullptr);
		}

		region.Fence->Signal(0);
		this->m_availableFences.push_back(region.Fence);

		head += region.UsedSize;

		this->m_inUseRegions.pop_front();
	}
}
