#include "pch.h"
#include "phxGfxRenderer.h"

#include "phxGfxDevice.h"

using namespace phx::gfx;

void phx::gfx::D3D12Renderer::SubmitCommands()
{
	const uint32_t numActiveCommands = this->m_activeCmdCount.exchange(0);

	D3D12Device* device = D3D12Device::Impl();
	for (size_t i = 0; i < (size_t)numActiveCommands; i++)
	{
		D3D12CommandList& commandList = this->m_commandListPool[i];
		commandList.m_commandList->Close();

		D3D12CommandQueue& queue = device->GetQueue(commandList.m_queueType);
		const bool dependency = !commandList.m_waits.empty() || !commandList.m_isWaitedOn;

		if (dependency)
			queue.Submit();

		queue.EnqueueForSubmit(commandList.m_commandList.Get(), commandList.m_allocator);
		commandList.m_allocator = nullptr;

		if (dependency)
		{
			// TODO;
		}
	}

	for (auto& q : device->GetQueues())
		q.Submit();
}

CommandList* phx::gfx::D3D12Renderer::BeginCommandRecording(CommandQueueType type)
{
	const uint32_t currentCmdIndex = this->m_activeCmdCount++;
	assert(currentCmdIndex < this->m_commandListPool.size());

	D3D12Device* device = D3D12Device::Impl();

	D3D12CommandQueue& queue = device->GetQueue(type);
	D3D12CommandList& cmdList = this->m_commandListPool[currentCmdIndex];
	cmdList.m_allocator = nullptr;
	cmdList.m_allocator= queue.RequestAllocator();
	if (cmdList.m_commandList == nullptr)
	{
		device->GetD3D12Device()->CreateCommandList(
			0,
			queue.Type,
			cmdList.m_allocator,
			nullptr,
			IID_PPV_ARGS(&cmdList.m_commandList));

		cmdList.m_commandList->SetName(L"D3D12GfxDevice::CommandList");
		dx::ThrowIfFailed(
			cmdList.m_commandList.As<ID3D12GraphicsCommandList6>(
				&cmdList.m_commandList6));

	}
	else
	{
		cmdList.m_commandList->Reset(cmdList.m_allocator, nullptr);
	}

	cmdList.m_id = currentCmdIndex;
	cmdList.m_queueType = type;
	cmdList.m_waits.clear();
	cmdList.m_isWaitedOn.store(false);

	// Bind Heaps
	std::array<ID3D12DescriptorHeap*, 2> heaps;
	Span<GpuDescriptorHeap> gpuHeaps = device->GetGpuDescriptorHeaps();
	for (int i = 0; i < gpuHeaps.Size(); i++)
	{
		heaps[i] = gpuHeaps[i].GetNativeHeap();
	}

	cmdList.m_commandList6->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

	cmdList.m_activeRenderPass.Initialize(cmdList.m_commandList.Get(), cmdList.m_commandList6.Get());
    return &cmdList;
}

RenderPassRenderer* phx::gfx::D3D12CommandList::BeginRenderPass()
{
	this->m_activeRenderPass.Initialize(this->GetD3D12CmdList(), this->GetD3D12CmdList6());
	return &this->m_activeRenderPass;
}

void phx::gfx::D3D12CommandList::EndRenderPass(RenderPassRenderer* pass)
{
}

void phx::gfx::D3D12RenderPassRenderer::Initialize(ID3D12GraphicsCommandList* commandList, ID3D12GraphicsCommandList6* commandList6)
{
	this->m_commandList = commandList;
	this->m_commadList6 = commandList6;
}

void phx::gfx::D3D12RenderPassRenderer::Begin(D3D12_CPU_DESCRIPTOR_HANDLE rtvView)
{
	CD3DX12_TEXTURE_BARRIER barrier()
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	this->m_commandList->ResourceBarrier(1, )
	float clearColour[] = {0.528f, 0.808f, 0.922f, 1.0f};
	this->m_commadList6->ClearRenderTargetView(rtvView, clearColour, 0, nullptr);
}
