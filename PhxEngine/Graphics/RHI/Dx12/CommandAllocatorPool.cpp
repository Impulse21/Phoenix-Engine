#include "phxpch.h"
#include "CommandAllocatorPool.h"

using namespace PhxEngine::RHI::Dx12;

CommandAllocatorPool::CommandAllocatorPool(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE type)
	: m_type(type)
	, m_device(device)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> clearQueue;
	std::swap(this->m_availableAllocators, clearQueue);

	this->m_allocatorPool.clear();
}

ID3D12CommandAllocator* PhxEngine::RHI::Dx12::CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
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
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> newAllocator;
		ThrowIfFailed(
			this->m_device->CreateCommandAllocator(
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

void CommandAllocatorPool::DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator)
{
	std::lock_guard<std::mutex> lockGuard(this->m_allocatonMutex);

	this->m_availableAllocators.push(std::make_pair(fence, allocator));
}
