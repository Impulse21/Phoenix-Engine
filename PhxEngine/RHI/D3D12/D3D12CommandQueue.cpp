
#define NOMINMAX
#include "D3D12CommandQueue.h"
#include "D3D12Device.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI::D3D12;

void D3D12CommandQueue::Initialize(D3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	this->m_device = device;
	this->m_type = type;
	this->m_allocatorPool.Initialize(device, type);

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = this->m_type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(
		this->m_device->GetNativeDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&this->m_d3d12CommandQueue)));

	// Create Fence
	ThrowIfFailed(
		this->m_device->GetNativeDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_d3d12Fence)));
	this->m_d3d12Fence->SetName(L"D3D12CommandQueue::Fence");

	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		this->m_d3d12CommandQueue->SetName(L"Direct Command Queue");
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		this->m_d3d12CommandQueue->SetName(L"Copy Command Queue");
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		this->m_d3d12CommandQueue->SetName(L"Compute Command Queue");
		break;
	}

#if 0
	this->m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_fenceEvent && "Failed to create fence event handle.");
#endif
	this->m_lastCompletedFenceValue = this->m_d3d12Fence->GetCompletedValue();
	this->m_nextFenceValue = this->m_lastCompletedFenceValue + 1;
}

void D3D12CommandQueue::Finalize()
{
	if (!this->m_d3d12CommandQueue)
	{
		return;
	}

#if 0
	CloseHandle(this->m_fenceEvent);
#endif
}

D3D12CommandList* PhxEngine::RHI::D3D12::D3D12CommandQueue::RequestCommandList(ID3D12CommandAllocator* allocator)
{
	std::scoped_lock _(this->m_commandListMutex);

	D3D12CommandList* commandList = nullptr;
	if (!this->m_commandListQueue.empty())
	{
		commandList = this->m_commandListQueue.front();
		this->m_commandListQueue.pop_front();
	}

	if (commandList == nullptr)
	{
		RefCountPtr<ID3D12GraphicsCommandList> commandList = nullptr;
		this->m_device->GetNativeDevice5()->CreateCommandList1(
			0,
			this->m_type,
			D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&commandList));
		commandList->SetName(L"D3D12GfxDevice::CommandList");

		commandList->Close();
		commandList = this->m_commandListPool.emplace_back(std::make_unique<D3D12CommandList>(commandList, this)).get();
	}

	commandList->Reset(allocator);
	return commandList;
}

D3D12CommandList& PhxEngine::RHI::D3D12::D3D12CommandQueue::BeginCommandList()
{
	ID3D12CommandAllocator* allocator = this->RequestAllocator();
	return *this->RequestCommandList(allocator);
}

uint64_t PhxEngine::RHI::D3D12::D3D12CommandQueue::ExecuteCommandLists(Core::Span<D3D12CommandList*> commandLists, bool waitForComplete)
{
	// Allocate and submit data
	static thread_local std::vector<ID3D12CommandList*> d3d12CommandLists;
	d3d12CommandLists.clear();

	for (auto commandList : commandLists)
	{
		d3d12CommandLists.push_back(commandList->NativeCmdList);
	}

	this->m_d3d12CommandQueue->ExecuteCommandLists(d3d12CommandLists.size(), d3d12CommandLists.data());
	uint64_t fenceVal = this->IncrementFence();

	for (auto commandList : commandLists)
	{
		this->m_commandListQueue.push_back(commandList);
	}

	return fenceVal;
}

uint64_t PhxEngine::RHI::D3D12::D3D12CommandQueue::ExecuteCommandList(D3D12CommandList& commandList, bool waitForComplete)
{
	return this->ExecuteCommandLists({ &commandList }, waitForComplete);
}


ID3D12CommandAllocator* PhxEngine::RHI::D3D12::D3D12CommandQueue::RequestAllocator()
{
	auto lastCompletedFence = this->GetLastCompletedFence();
	return this->m_allocatorPool.RequestAllocator(lastCompletedFence);
}

void PhxEngine::RHI::D3D12::D3D12CommandQueue::DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator)
{
	this->m_allocatorPool.DiscardAllocator(fence, allocator);
}

uint64_t PhxEngine::RHI::D3D12::D3D12CommandQueue::IncrementFence()
{
	std::scoped_lock _(this->m_fenceMutex);
	this->m_d3d12CommandQueue->Signal(this->m_d3d12Fence.Get(), this->m_nextFenceValue);
	return this->m_nextFenceValue++;
}

bool PhxEngine::RHI::D3D12::D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (fenceValue > this->m_lastCompletedFenceValue)
	{
		this->m_lastCompletedFenceValue = std::max(this->m_lastCompletedFenceValue, this->m_d3d12Fence->GetCompletedValue());
	}

	return fenceValue <= this->m_lastCompletedFenceValue;
}

void PhxEngine::RHI::D3D12::D3D12CommandQueue::WaitForFence(uint64_t fenceValue)
{
	if (this->IsFenceComplete(fenceValue))
	{
		return;
	}
	{
		// Wait on the frames last value?
		// NULL event handle will simply wait immediately:
		//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
		this->m_d3d12Fence->SetEventOnCompletion(fenceValue, nullptr);
		this->m_lastCompletedFenceValue = fenceValue;
	}
}

uint64_t PhxEngine::RHI::D3D12::D3D12CommandQueue::GetLastCompletedFence()
{
	std::scoped_lock _(this->m_fenceMutex);
	return this->m_d3d12Fence->GetCompletedValue();
}

void PhxEngine::RHI::D3D12::D3D12CommandQueue::CommandAllocatorPool::Initialize(D3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	this->m_type = type;
	this->m_device = device;
}


ID3D12CommandAllocator* PhxEngine::RHI::D3D12::D3D12CommandQueue::CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
{
	std::scoped_lock _(this->m_allocatonMutex);

	ID3D12CommandAllocator* pAllocator = nullptr;
	if (!this->m_availableAllocators.empty())
	{
		auto& allocatorPair = this->m_availableAllocators.front();
		if (allocatorPair.first <= completedFenceValue)
		{
			pAllocator = allocatorPair.second;
			ThrowIfFailed(
				pAllocator->Reset());

			this->m_availableAllocators.pop();
		}
	}

	if (!pAllocator)
	{
		Core::RefCountPtr<ID3D12CommandAllocator> newAllocator;
		ThrowIfFailed(
			this->m_device->GetNativeDevice()->CreateCommandAllocator(
				this->m_type,
				IID_PPV_ARGS(&newAllocator)));

		// TODO: std::shared_ptr is leaky
		wchar_t allocatorName[32];
		swprintf(allocatorName, 32, L"CommandAllocator %zu", this->m_allocatorPool.size());
		newAllocator->SetName(allocatorName);
		this->m_allocatorPool.emplace_back(newAllocator);
		pAllocator = this->m_allocatorPool.back().Get();
	}

	return pAllocator;
}


void PhxEngine::RHI::D3D12::D3D12CommandQueue::CommandAllocatorPool::DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator)
{
	std::scoped_lock _(this->m_allocatonMutex);

	assert(allocator);
	this->m_availableAllocators.push(std::make_pair(fence, allocator));
}