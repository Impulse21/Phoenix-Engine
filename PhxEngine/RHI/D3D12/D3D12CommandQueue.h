#pragma once

#include <queue>
#include <mutex>

#include <d3d12.h>
#include <Core/RefCountPtr.h>
#include <Core/Span.h>

namespace PhxEngine::RHI::D3D12
{
	class D3D12Device;
	class D3D12CommandQueue
	{
	public:
		D3D12CommandQueue() = default;
		~D3D12CommandQueue() = default;

		void Initialize(D3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
		void Finalize();

		D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

		uint64_t ExecuteCommandLists(Core::Span<ID3D12CommandList*> commandLists, bool waitForComplete = false);

		ID3D12CommandAllocator* RequestAllocator();
		void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		uint64_t IncrementFence();
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFence(uint64_t fenceValue);
		void WaitForIdle() { this->WaitForFence(this->IncrementFence()); }

		operator ID3D12CommandQueue* () const { return this->m_d3d12CommandQueue.Get(); }

		ID3D12CommandQueue* GetD3D12CommandQueue() { return this->m_d3d12CommandQueue.Get(); }
		ID3D12Fence* GetFence() { return this->m_d3d12Fence.Get(); }
		uint64_t GetLastCompletedFence();

	private:
		D3D12_COMMAND_LIST_TYPE m_type;
		D3D12Device* m_device;

		Core::RefCountPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
		Core::RefCountPtr<ID3D12Fence> m_d3d12Fence;

		class CommandAllocatorPool
		{
		public:
			void Initialize(D3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
			ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
			void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		private:
			std::vector<Core::RefCountPtr<ID3D12CommandAllocator>> m_allocatorPool;
			std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
			std::mutex m_allocatonMutex;
			D3D12Device* m_device;
			D3D12_COMMAND_LIST_TYPE m_type;
		} m_allocatorPool;

		uint64_t m_nextFenceValue = 0;
		uint64_t m_lastCompletedFenceValue = 0;

		std::mutex m_fenceMutex;
	};
}