#pragma once

#include <queue>
#include <mutex>

#include "D3D12Common.h"
namespace PhxEngine::RHI::D3D12
{

	class D3D12Device;
	class D3D12CommandAllocatorPool
	{
	public:
		D3D12CommandAllocatorPool(
			D3D12Device* device,
			D3D12_COMMAND_LIST_TYPE type);
		~D3D12CommandAllocatorPool();

		ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
		void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);


		inline size_t Size() { return this->m_allocatorPool.size(); }

	private:
		D3D12Device* m_device;
		const D3D12_COMMAND_LIST_TYPE m_type;

		std::vector<RefCountPtr<ID3D12CommandAllocator>> m_allocatorPool;
		std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
		std::mutex m_allocatonMutex;
	};

	class D3D12CommandList : public RefCountPtr<IRHICommandList>
	{
	public:
		D3D12CommandList() = default;
		~D3D12CommandList() = default;

		void Reset(ID3D12CommandAllocator* allocator);

		void Executed();
	};
}

