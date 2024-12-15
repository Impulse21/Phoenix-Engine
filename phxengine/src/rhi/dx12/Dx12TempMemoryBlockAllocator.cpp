#include "pch.h"
#include "phx/rhi/dx12/Dx12TempMemoryBlockAllocator.h"

using namespace phx::rhi::dx12;

void TempMemoryBlockAllocator::Initialize(GfxDeviceDx12& device, dx12::GpuBufferBindings* bindings)
{
}

void TempMemoryBlockAllocator::EndFrame(GfxDeviceDx12& device)
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
		device.GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(newFence.ReleaseAndGetAddressOf()));
		this->m_fencePool.push_back(newFence);
		fence = newFence.Get();
	}

	q->Signal(fence, 1);
	this->m_inUseRegions.push_front(UsedRegion{
		.UsedSize = m_tail - m_headAtStartOfFrame,
		.Fence = fence });
}

void TempMemoryBlockAllocator::Finalize()
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
}