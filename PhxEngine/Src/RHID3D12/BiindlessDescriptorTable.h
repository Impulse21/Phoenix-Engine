#pragma once

#include "D3D12Common.h"

#include "DescriptorHeap.h"

#include <PhxEngine/RHI/PhxRHI.h>
#include <queue>

namespace PhxEngine::RHI::D3D12
{
	class BindlessDescriptorTable
	{
	public:
		BindlessDescriptorTable(DescriptorHeapAllocation&& allocation)
			: m_allocation(std::move(allocation))
		{
		}

		// TODO Free allocation
		~BindlessDescriptorTable() = default;

		BindlessDescriptorTable(const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable(BindlessDescriptorTable&&) = delete;
		BindlessDescriptorTable& operator = (const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable& operator = (BindlessDescriptorTable&&) = delete;

	public:
		DescriptorIndex Allocate();
		void Free(DescriptorIndex index);

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