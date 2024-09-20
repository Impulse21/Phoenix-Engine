#include "pch.h"
#include "phxGfxRenderer.h"

#include "phxGfxDevice.h"

using namespace phx::gfx;

void phx::gfx::D3D12Renderer::SubmitCommands()
{
}

CommandList* phx::gfx::D3D12Renderer::BeginCommandRecording(CommandQueueType type)
{
	const uint32_t currentCmdIndex = this->m_activeCmdCount++;
	assert(currentCmdIndex < this->m_commandListPool.size());

	D3D12Device* device = D3D12Device::Impl();

	D3D12CommandQueue& queue = device->GetQueue(type);
	D3D12CommandList& cmdList = this->m_commandListPool[currentCmdIndex];
#if false
	cmdList.NativeCommandAllocator = nullptr;
	cmdList.NativeCommandAllocator = queue.RequestAllocator();
	if (cmdList.NativeCommandList == nullptr)
	{
		this->GetD3D12Device()->CreateCommandList(
			0,
			this->GetQueue(queueType).GetType(),
			cmdList.NativeCommandAllocator,
			nullptr,
			IID_PPV_ARGS(&cmdList.NativeCommandList));

		cmdList.NativeCommandList->SetName(L"D3D12GfxDevice::CommandList");
		ThrowIfFailed(
			cmdList.NativeCommandList.As<ID3D12GraphicsCommandList6>(
				&cmdList.NativeCommandList6));

	}
	else
	{
		cmdList.NativeCommandList->Reset(cmdList.NativeCommandAllocator, nullptr);
	}

	cmdList.Id = currentCmdIndex;
	cmdList.QueueType = queueType;
	cmdList.Waits.clear();
	cmdList.IsWaitedOn.store(false);
	cmdList.UploadBuffer.Reset();

	// Bind Heaps
	std::array<ID3D12DescriptorHeap*, 2> heaps;
	for (int i = 0; i < this->m_gpuDescriptorHeaps.size(); i++)
	{
		heaps[i] = this->m_gpuDescriptorHeaps[i].GetNativeHeap();
	}
	cmdList.NativeCommandList6->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
#endif

    return nullptr;
}
