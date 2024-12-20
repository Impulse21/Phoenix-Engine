#include "phx/pch.h"
#include "phx/rhi/dx12/Dx12TempMemoryBlockAllocator.h"

using namespace phx::rhi::dx12;

void TempMemoryBlockAllocator::Initialize(GfxDeviceDx12&, dx12::GpuBufferBindings*)
{
}

void TempMemoryBlockAllocator::EndFrame(GfxDeviceDx12& device, uint32_t usedSize, uint32_t& head)
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

		head += region.UsedSize;

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
		device.GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(newFence.ReleaseAndGetAddressOf()));
		this->m_fencePool.push_back(newFence);
		fence = newFence.Get();
	}

	device.GetGfxQueue().Queue->Signal(fence, 1);
	this->m_inUseRegions.push_front(UsedRegion{
		.UsedSize = usedSize,
		.Fence = fence });
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
}
