#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "CommandQueue.h"
#include "D3D12GraphicsDevice.h"

using namespace Microsoft::WRL;
using namespace PhxEngine::RHI::D3D12;

CommandQueue::CommandQueue(D3D12GraphicsDevice& graphicsDevice, D3D12_COMMAND_LIST_TYPE type)
	: m_graphicsDevice(graphicsDevice)
	, m_type(type)
	, m_allocatorPool(graphicsDevice.GetD3D12Device2(), type)
{
	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = this->m_type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(
		this->m_graphicsDevice.GetD3D12Device2()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&this->m_d3d12CommandQueue)));

	// Create Fence
	ThrowIfFailed(
		this->m_graphicsDevice.GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_d3d12Fence)));
	this->m_d3d12Fence->SetName(L"Dx12CommandQueue::Dx12CommandQueue::Fence");

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

	this->m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_fenceEvent && "Failed to create fence event handle.");

	this->m_lastCompletedFenceValue = this->m_d3d12Fence->GetCompletedValue();
	this->m_nextFenceValue = this->m_lastCompletedFenceValue + 1;
}

PhxEngine::RHI::D3D12::CommandQueue::~CommandQueue()
{
	if (!this->m_d3d12CommandQueue)
	{
		return;
	}

	CloseHandle(this->m_fenceEvent);
}

D3D12CommandList* PhxEngine::RHI::D3D12::CommandQueue::RequestCommandList()
{
	auto _ = std::scoped_lock(this->m_commandListMutx);

	D3D12CommandList* retVal;
	if (!this->m_availableCommandLists.empty())
	{
		retVal = this->m_availableCommandLists.front();
	}
	else
	{
		retVal = this->m_commandListsPool.emplace_back(std::make_unique<D3D12CommandList>(this)).get();
	}

	return retVal;
}

uint64_t PhxEngine::RHI::D3D12::CommandQueue::ExecuteCommandLists(Core::Span<D3D12CommandList*> commandLists)
{
	auto _ = std::scoped_lock(this->m_commandListMutx);
	static thread_local std::vector<ID3D12CommandList*> d3d12CommandLists;
	d3d12CommandLists.clear();
	
	for (auto commandList : commandLists)
	{
		d3d12CommandLists.push_back(commandList->GetD3D12CommandList());
	}

	uint64_t fenceVal= this->ExecuteCommandLists(d3d12CommandLists);

	for (auto& commandList : commandLists)
	{
		commandList->Executed(fenceVal);
		this->m_availableCommandLists.emplace(commandList);
	}

	return fenceVal;
}

uint64_t PhxEngine::RHI::D3D12::CommandQueue::ExecuteCommandLists(std::vector<ID3D12CommandList*> const& commandLists)
{
	this->m_d3d12CommandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());

	bool isRemoved = IGraphicsDevice::GPtr->IsDevicedRemoved();
	if (isRemoved)
	{
		// Only capture if the Device was removed
		IGraphicsDevice::GPtr->EndCapture();
	}

	return this->IncrementFence();
}

ID3D12CommandAllocator* PhxEngine::RHI::D3D12::CommandQueue::RequestAllocator()
{
	auto lastCompletedFence = this->GetLastCompletedFence();
	return this->m_allocatorPool.RequestAllocator(lastCompletedFence);
}

void PhxEngine::RHI::D3D12::CommandQueue::DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator)
{
	this->m_allocatorPool.DiscardAllocator(fence, allocator);
}

uint64_t PhxEngine::RHI::D3D12::CommandQueue::IncrementFence()
{
	std::scoped_lock _(this->m_fenceMutex);
	this->m_d3d12CommandQueue->Signal(this->m_d3d12Fence.Get(), this->m_nextFenceValue);
	return this->m_nextFenceValue++;
}

bool PhxEngine::RHI::D3D12::CommandQueue::IsFenceComplete(uint64_t fenceValue)
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

void PhxEngine::RHI::D3D12::CommandQueue::WaitForFence(uint64_t fenceValue)
{
	if (this->IsFenceComplete(fenceValue))
	{
		return;
	}

	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		std::scoped_lock _(this->m_eventMutex);

		this->m_d3d12Fence->SetEventOnCompletion(fenceValue, this->m_fenceEvent);
		WaitForSingleObject(this->m_fenceEvent, INFINITE);
		this->m_lastCompletedFenceValue = fenceValue;
	}
}

uint64_t PhxEngine::RHI::D3D12::CommandQueue::GetLastCompletedFence()
{
	std::scoped_lock _(this->m_fenceMutex);
	return this->m_d3d12Fence->GetCompletedValue();
}