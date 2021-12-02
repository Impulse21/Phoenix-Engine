#pragma once

#include <queue>
#include <mutex>

#include "Common.h"

namespace PhxEngine::RHI::Dx12
{
	class CommandAllocatorPool
	{
	public:
		CommandAllocatorPool(
			RefCountPtr<ID3D12Device2> device,
			D3D12_COMMAND_LIST_TYPE type);
		~CommandAllocatorPool();

		ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
		void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);


		inline size_t Size() { return this->m_allocatorPool.size(); }

	private:
		RefCountPtr<ID3D12Device2> m_device;
		const D3D12_COMMAND_LIST_TYPE m_type;

		std::vector<RefCountPtr<ID3D12CommandAllocator>> m_allocatorPool;
		std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
		std::mutex m_allocatonMutex;
	};
}