#pragma once

#include <queue>
#include <mutex>

#include "D3D12Common.h"
#include "CommandAllocatorPool.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12GraphicsDevice;
	class D3D12CommandList;
	class CommandQueue
	{
	public:
		CommandQueue(D3D12GraphicsDevice& graphicsDevice, D3D12_COMMAND_LIST_TYPE type);
		~CommandQueue();

		D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

		D3D12CommandList* RequestCommandList();
		uint64_t ExecuteCommandLists(Core::Span<D3D12CommandList*> commandLists);
		// TODO: Move to private.
		uint64_t ExecuteCommandLists(std::vector<ID3D12CommandList*> const& commandLists);

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

		D3D12GraphicsDevice& GetGfxDevice() { return this->m_graphicsDevice; }

	private:
		const D3D12_COMMAND_LIST_TYPE m_type;
		D3D12GraphicsDevice& m_graphicsDevice;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_d3d12Fence;
		
		std::vector<std::unique_ptr<D3D12CommandList>> m_commandListsPool;
		std::queue<D3D12CommandList*> m_availableCommandLists;
		std::mutex m_commandListMutx;

		CommandAllocatorPool m_allocatorPool;

		uint64_t m_nextFenceValue = 0;
		uint64_t m_lastCompletedFenceValue = 0;

		std::mutex m_fenceMutex;
		std::mutex m_eventMutex;
		HANDLE m_fenceEvent;
	};
}