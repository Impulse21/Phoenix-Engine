#pragma once

#include <queue>
#include <mutex>

#include "D3D12Common.h"
#include <PhxEngine/Core/Span.h>

namespace PhxEngine::RHI::D3D12
{
	class D3D12CommandQueue
	{
	public:
		D3D12CommandQueue() = default;
		~D3D12CommandQueue() = default;

		void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type);
		void Finailize();

		D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

#if 0 
		RequestCommandContext();
		uint64_t ExecuteCommandContexts(Core::Span<D3D12CommandContext*> contexts);
#endif

		ID3D12CommandAllocator* RequestAllocator();
		void DiscardAllocators(uint64_t fence, Core::Span<ID3D12CommandAllocator*> allocators);
		void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		uint64_t IncrementFence();
		uint64_t GetNextFenceValue() { return this->m_nextFenceValue + 1; }
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFence(uint64_t fenceValue);
		void WaitForIdle() { this->WaitForFence(this->IncrementFence()); }

		operator ID3D12CommandQueue* () const { return this->m_d3d12CommandQueue.Get(); }

		ID3D12CommandQueue* GetD3D12CommandQueue() { return this->m_d3d12CommandQueue.Get(); }
		ID3D12Fence* GetFence() { return this->m_d3d12Fence.Get(); }
		uint64_t GetLastCompletedFence();
				
		uint64_t ExecuteCommandLists(Core::Span<ID3D12CommandList*> commandLists);

	private:
		D3D12_COMMAND_LIST_TYPE m_type;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_d3d12Fence;
		
#if 0
		std::vector<std::unique_ptr<D3D12CommandContext>> m_commandListsPool;
		std::queue<D3D12CommandContext*> m_availableCommandLists;
#endif
		std::mutex m_commandListMutx;
		class CommandAllocatorPool
		{
		public:
			void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type);
			ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
			void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocatorPool;
			std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
			std::mutex m_allocatonMutex;
			ID3D12Device* m_nativeDevice;
			D3D12_COMMAND_LIST_TYPE m_type;
		} m_allocatorPool;

		uint64_t m_nextFenceValue = 0;
		uint64_t m_lastCompletedFenceValue = 0;

		std::mutex m_fenceMutex;
		std::mutex m_eventMutex;
		HANDLE m_fenceEvent;
	};
}