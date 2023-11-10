#pragma once

#include <queue>
#include <mutex>

#include <d3d12.h>
#include <Core/RefCountPtr.h>
#include <Core/Span.h>
#include <RHI/D3D12/D3D12CommandList.h>
namespace PhxEngine::RHI::D3D12
{
	struct D3D12CommandList;
	class D3D12Context;
	class D3D12CommandQueue final
	{
	public:
		D3D12CommandQueue() = default;
		~D3D12CommandQueue() = default;

		void Initialize(Core::RefCountPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type);
		void Finalize();

		D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

		D3D12CommandList& BeginCommandList();
		uint64_t ExecuteCommandLists(Core::Span<D3D12CommandList*> commandLists, bool waitForComplete = false);
		uint64_t ExecuteCommandList(D3D12CommandList& commandList, bool waitForComplete = false);

		uint64_t IncrementFence();
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFence(uint64_t fenceValue);
		void WaitForIdle() { this->WaitForFence(this->IncrementFence()); }

		operator ID3D12CommandQueue* () const { return this->m_d3d12CommandQueue.Get(); }

		ID3D12CommandQueue* GetD3D12CommandQueue() { return this->m_d3d12CommandQueue.Get(); }
		ID3D12Fence* GetFence() { return this->m_d3d12Fence.Get(); }
		uint64_t GetLastCompletedFence();

	protected:
		D3D12CommandList* RequestCommandList(ID3D12CommandAllocator* allocator);
		ID3D12CommandAllocator* RequestAllocator();
		void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

	private:
		D3D12_COMMAND_LIST_TYPE m_type;
		Core::RefCountPtr<ID3D12Device> m_device;

		Core::RefCountPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
		Core::RefCountPtr<ID3D12Fence> m_d3d12Fence;

		using CommandListPool = std::vector<std::unique_ptr<D3D12CommandList>>;
		using CommandListQueue = std::deque<D3D12CommandList*>;
		std::mutex m_commandListMutex;
		CommandListPool m_commandListPool;
		CommandListQueue m_commandListQueue;

		class CommandAllocatorPool
		{
		public:
			void Initialize(Core::RefCountPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type);
			ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
			void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		private:
			std::vector<Core::RefCountPtr<ID3D12CommandAllocator>> m_allocatorPool;
			std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
			std::mutex m_allocatonMutex;
			Core::RefCountPtr<ID3D12Device> m_device;
			D3D12_COMMAND_LIST_TYPE m_type;
		} m_allocatorPool;

		uint64_t m_nextFenceValue = 0;
		uint64_t m_lastCompletedFenceValue = 0;

		std::mutex m_fenceMutex;
	};
}