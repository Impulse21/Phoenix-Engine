#pragma once

#include <queue>
#include <mutex>

#include "Common.h"
#include "CommandAllocatorPool.h"

namespace PhxEngine::RHI::Dx12
{
	class GraphicsDevice;

	class CommandQueue
	{
	public:
		CommandQueue(GraphicsDevice& graphicsDevice, D3D12_COMMAND_LIST_TYPE type);
		~CommandQueue();

		D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

		uint64_t ExecuteCommandLists(std::vector<ID3D12CommandList*> const& commandLists);

		uint64_t IncrementFence();
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFence(uint64_t fenceValue);
		void WaitForIdle() { this->WaitForFence(this->IncrementFence()); }

		operator ID3D12CommandQueue* () const { return this->m_d3d12CommandQueue.Get(); }

		ID3D12CommandQueue* GetD3D12CommandQueue() { return this->m_d3d12CommandQueue.Get(); }

		uint64_t GetLastCompletedFence();

	private:
		const D3D12_COMMAND_LIST_TYPE m_type;
		GraphicsDevice& m_graphicsDevice;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_d3d12Fence;
		
		CommandAllocatorPool m_allocatorPool;

		uint64_t m_nextFenceValue = 0;
		uint64_t m_lastCompletedFenceValue = 0;

		std::mutex m_fenceMutex;
		std::mutex m_eventMutex;
		HANDLE m_fenceEvent;
	};
}