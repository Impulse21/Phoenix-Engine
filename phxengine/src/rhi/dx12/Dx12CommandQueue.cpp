#include "pch.h"

#include "Dx12CommandQueue.h"
#include "Dx12GfxDevice.h"
using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::dx12;

ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
{
	SCOPED_LOCK(MutexAllocation);

	const uint64_t completedFenceValue = Fence->GetCompletedValue();
	ID3D12CommandAllocator* retVal = nullptr;
	if (!AvailableAllocators.empty())
	{
		auto& [fenceValue, allocator] = AvailableAllocators.front();
		if (fenceValue < completedFenceValue)
		{
			retVal = allocator;
			retVal->Reset();
			AvailableAllocators.pop_front();
		}
	}

	if (!retVal)
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& newAllocator = AllocatorPool.emplace_back();
		ThrowIfFailed(
			GfxDeviceDx12::Instance()->GetD3D12Device2()->CreateCommandAllocator(Type, IID_PPV_ARGS(&newAllocator)));

		newAllocator->SetName(L"Allocator");
		retVal = newAllocator.Get();
	}

	return retVal;
}