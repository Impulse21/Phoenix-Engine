#pragma once

#include "phx/core/Span.h"

#include "D3D12Core.h"

#include <deque>
#include <mutex>

namespace phx::rhi::d3d12
{
	struct D3D12CommandQueue
	{
		D3D12_COMMAND_LIST_TYPE Type;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;

		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> AllocatorPool;
		std::deque<std::pair<uint64_t, ID3D12CommandAllocator*>> AvailableAllocators;

		std::vector<ID3D12CommandList*> m_pendingSubmitCommands;
		std::vector<ID3D12CommandAllocator*> m_pendingSubmitAllocators;

		std::mutex MutexAllocation;
		uint64_t NextFenceValue = 0;

		void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type)
		{
			Type = type;

			// Create Command Queue
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = Type;
			queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.NodeMask = 0;


			ThrowIfFailed(
				nativeDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&Queue)));

			// Create Fence
			ThrowIfFailed(
				nativeDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
			Fence->SetName(L"D3D12CommandQueue::D3D12CommandQueue::Fence");

			NextFenceValue = Fence->GetCompletedValue() + 1;
			switch (type)
			{
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				Queue->SetName(L"Direct Command Queue");
				break;
			case D3D12_COMMAND_LIST_TYPE_COPY:
				Queue->SetName(L"Copy Command Queue");
				break;
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				Queue->SetName(L"Compute Command Queue");
				break;
			default:
				throw std::runtime_error("Unsupported Command list Type");
			}
		}

		uint64_t SubmitCommandLists(Span<ID3D12CommandList*> commandLists)
		{
			Queue->ExecuteCommandLists((UINT)commandLists.Size(), commandLists.begin());

			return IncrementFence();
		}

		void DiscardAllocators(uint64_t fenceValue, Span<ID3D12CommandAllocator*> allocators)
		{
			std::scoped_lock _(MutexAllocation);
			for (ID3D12CommandAllocator* allocator : allocators)
			{
				AvailableAllocators.push_back(std::make_pair(fenceValue, allocator));
			}
		}

		uint64_t IncrementFence()
		{
			Queue->Signal(Fence.Get(), NextFenceValue);
			return NextFenceValue++;
		}

		uint64_t Submit()
		{
			const uint64_t fenceValue = SubmitCommandLists(m_pendingSubmitCommands);
			m_pendingSubmitCommands.clear();

			DiscardAllocators(fenceValue, m_pendingSubmitAllocators);
			m_pendingSubmitAllocators.clear();

			return fenceValue;
		}

		void EnqueueForSubmit(ID3D12CommandList* cmdList, ID3D12CommandAllocator* allocator)
		{
			m_pendingSubmitAllocators.push_back(allocator);
			m_pendingSubmitCommands.push_back(cmdList);
		}

		ID3D12CommandAllocator* RequestAllocator();
	};
}