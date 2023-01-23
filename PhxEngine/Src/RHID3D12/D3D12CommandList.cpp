#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12CommandList.h"

#include "D3D12Device.h"
#include "D3D12RHI.h"

using namespace PhxEngine::RHI::D3D12;

D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(
	D3D12Device* device,
	D3D12_COMMAND_LIST_TYPE type)
	: m_type(type)
	, m_device(device)
{
}

D3D12CommandAllocatorPool::~D3D12CommandAllocatorPool()
{
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> clearQueue;
	std::swap(this->m_availableAllocators, clearQueue);

	this->m_allocatorPool.clear();
}

ID3D12CommandAllocator* PhxEngine::RHI::D3D12::D3D12CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
{
	std::lock_guard<std::mutex> lockGuard(this->m_allocatonMutex);

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
		RefCountPtr<ID3D12CommandAllocator> newAllocator;
		ThrowIfFailed(
			this->m_device->GetNativeDevice()->CreateCommandAllocator(
				this->m_type,
				IID_PPV_ARGS(&newAllocator)));

		// TODO: std::shared_ptr is leaky
		wchar_t allocatorName[32];
		swprintf(allocatorName, 32, L"CommandAllocator %zu", this->Size());
		newAllocator->SetName(allocatorName);
		this->m_allocatorPool.emplace_back(newAllocator);
		pAllocator = this->m_allocatorPool.back().Get();
	}

	return pAllocator;
}

void D3D12CommandAllocatorPool::DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator)
{
	std::lock_guard<std::mutex> lockGuard(this->m_allocatonMutex);

	this->m_availableAllocators.push(std::make_pair(fence, allocator));
}

void PhxEngine::RHI::D3D12::D3D12CommandList::Reset(ID3D12CommandAllocator* allocator)
{
}

void PhxEngine::RHI::D3D12::D3D12CommandList::Executed()
{
}
