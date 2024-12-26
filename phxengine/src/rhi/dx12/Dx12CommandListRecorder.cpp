#include "phx/phx_pch.h"

#include "phx/rhi/dx12/Dx12CommandListRecorder.h"
#include "phx/rhi/dx12/Dx12GfxDevice.h"

using namespace phx;
using namespace phx::rhi;

void dx12::GfxCommandListRecorder::Open(CommandListResource* resource)
{
	if (resource->Allocator)
		return; // Already open

	auto* device = dx12::GfxDeviceDx12::Instance();

	D3D12CommandQueue& queue = device->GetQueue(resource->Type);
	resource->Allocator = queue.RequestAllocator();

	if (resource->CmdList == nullptr)
	{
		device->GetD3D12Device()->CreateCommandList(
			0,
			queue.Type,
			resource->Allocator,
			nullptr,
			IID_PPV_ARGS(&resource->CmdList));

		resource->CmdList->SetName(L"GfxDeviceD3D12::CommandList");
		ThrowIfFailed(
			resource->CmdList.As<ID3D12GraphicsCommandList6>(
				&resource->CmdList6));
	}
	else
	{
		resource->CmdList->Reset(resource->Allocator, nullptr);
	}

	// Bind Heaps
	std::array<ID3D12DescriptorHeap*, 2> heaps;
	Span<GpuDescriptorHeap> gpuHeaps = device->GetGpuDescriptorHeaps();
	for (int i = 0; i < gpuHeaps.Size(); i++)
	{
		heaps[i] = gpuHeaps[i].GetNativeHeap();
	}

	resource->CmdList6->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}