#include "PhxRhi/PhxRhi_pch.h"

#include "D3D12CommandQueue.h"

#include "D3D12Core.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;

ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
{
	std::scoped_lock _(MutexAllocation);

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
			g_d3d12Device2->CreateCommandAllocator(Type, IID_PPV_ARGS(&newAllocator)));

		newAllocator->SetName(L"Allocator");
		retVal = newAllocator.Get();
	}

	return retVal;
}