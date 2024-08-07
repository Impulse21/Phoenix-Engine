#pragma once

#include "D3D12Common.h"
#include "D3D12DescriptorHeap.h"

#include <queue>

namespace phx::rhi::d3d12
{
	class D3D12BindlessDescriptorTable : NonCopyable, NonMoveable
	{
	public:

		void Initialize(DescriptorHeapAllocation&& allocation)
		{
			this->m_allocation = std::move(allocation);
		}

	public:
		DescriptorIndex Allocate()
		{
			return this->m_descriptorIndexPool.Allocate();
		}

		void Free(DescriptorIndex index)
		{
			this->m_descriptorIndexPool.Release(index);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DescriptorIndex index) const { return this->m_allocation.GetCpuHandle(index); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DescriptorIndex index) const { return this->m_allocation.GetGpuHandle(index); }

	private:
		struct DescriptorIndexPool
		{
			// Removes the first element from the free list and returns its index
			UINT Allocate()
			{
				std::scoped_lock Guard(this->IndexMutex);

				UINT NewIndex;
				if (!IndexQueue.empty())
				{
					NewIndex = IndexQueue.front();
					IndexQueue.pop();
				}
				else
				{
					NewIndex = Index++;
				}
				return NewIndex;
			}

			void Release(UINT Index)
			{
				std::scoped_lock Guard(this->IndexMutex);
				IndexQueue.push(Index);
			}

			std::mutex IndexMutex;
			std::queue<DescriptorIndex> IndexQueue;
			size_t Index = 0;
		};

	private:
		DescriptorHeapAllocation m_allocation;
		DescriptorIndexPool m_descriptorIndexPool;

	};
}