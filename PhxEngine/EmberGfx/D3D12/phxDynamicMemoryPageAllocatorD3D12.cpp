#include "pch.h"
#include "phxDynamicMemoryPageAllocatorD3D12.h"

#include "phxGfxDeviceD3D12.h"

using namespace phx::gfx;

void phx::gfx::GpuRingAllocator::Initialize(size_t bufferSize)
{
	assert((bufferSize & (bufferSize - 1)) == 0);
	this->m_bufferMask = (bufferSize - 1);

	this->m_buffer = GfxDeviceD3D12::CreateBuffer({
			.Usage = Usage::Upload,
			.SizeInBytes = bufferSize,
			.DebugName = "Upload Buffer"
		});

	D3D12Buffer* bufferImpl = GfxDeviceD3D12::GetRegistry().Buffers.Get(this->m_buffer);
	this->m_data = reinterpret_cast<uint8_t*>(bufferImpl->MappedData);
	this->m_gpuAddress = bufferImpl->D3D12Resource->GetGPUVirtualAddress();
}

void phx::gfx::GpuRingAllocator::Finalize()
{
	while (!this->m_inUseRegions.empty())
	{
		auto& region = this->m_inUseRegions.front();
		if (region.Fence->GetCompletedValue() != 1)
		{
			region.Fence->SetEventOnCompletion(1, NULL);
		}

		this->m_inUseRegions.pop_front();
	}

	this->m_data = nullptr;
	this->m_gpuAddress = 0;
	this->m_fencePool.clear();

	this->m_inUseRegions.clear();
	this->m_availableFences.clear();


	GfxDeviceD3D12::DeleteResource(this->m_buffer);
}

void phx::gfx::GpuRingAllocator::EndFrame(ID3D12CommandQueue* q)
{

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
		GfxDeviceD3D12::GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(newFence.ReleaseAndGetAddressOf()));
		this->m_fencePool.push_back(newFence);
		fence = newFence.Get();
	}

	q->Signal(fence, 1);
	this->m_inUseRegions.push_front(UsedRegion{
		.UsedSize = m_tail - m_headAtStartOfFrame,
		.Fence = fence });

	m_headAtStartOfFrame = m_tail;
}

DynamicMemoryPage phx::gfx::GpuRingAllocator::Allocate(uint32_t allocSize)
{
	std::scoped_lock _(this->m_mutex);

	// Checks if the top bits have changes, if so, we need to wrap around.
	if ((m_tail ^ (m_tail + allocSize)) & ~m_bufferMask)
	{
		m_tail = (m_tail + m_bufferMask) & ~m_bufferMask;
	}

	if (((m_tail - m_head) + allocSize) >= GetBufferSize())
	{
		while (!this->m_inUseRegions.empty())
		{
			auto& region = this->m_inUseRegions.front();
			if (region.Fence->GetCompletedValue() != 1)
			{
				PHX_CORE_WARN("[GPU QUEUE] Stalling waiting for space");
				region.Fence->SetEventOnCompletion(1, NULL);
			}

			region.Fence->Signal(0);
			this->m_availableFences.push_back(region.Fence);

			m_head += region.UsedSize;

			this->m_inUseRegions.pop_front();
		}
	}

	const uint32_t offset = (this->m_tail & m_bufferMask) * allocSize;
	m_tail++;

	return DynamicMemoryPage{
		.BufferHandle = this->m_buffer,
		.GpuAddress = this->m_gpuAddress + offset,
		.Data = reinterpret_cast<uint8_t*>(this->m_data + offset),
	};
}
