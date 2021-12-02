#pragma once

#include "Common.h"

#include "DescriptorHeap.h"

namespace PhxEngine::RHI::Dx12
{
	class BindlessDescriptorTable
	{
	public:
		BindlessDescriptorTable(DescriptorHeapAllocation&& allocation)
			: m_allocation(std::move(allocation))
		{}

		// TODO Free allocation
		~BindlessDescriptorTable() = default;

		BindlessDescriptorTable(const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable(BindlessDescriptorTable&&) = delete;
		BindlessDescriptorTable& operator = (const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable& operator = (BindlessDescriptorTable&&) = delete;

	public:
		DescriptorIndex Allocate();
		void Free(DescriptorIndex index);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DescriptorIndex index) { return this->m_allocation.GetCpuHandle(index); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DescriptorIndex index) { return this->m_allocation.GetGpuHandle(index); }

	private:
		class DescriptorIndexPool
		{
		public:
			DescriptorIndexPool() = default;
			DescriptorIndexPool(size_t numIndices)
			{
				this->m_elements.resize(numIndices);
				Reset();
			}

			auto& operator[](size_t index) { return this->m_elements[index]; }

			const auto& operator[](size_t index) const { return this->m_elements[index]; }

			void Reset()
			{
				this->m_freeStart = 0;
				this->m_numActiveElements = 0;
				for (size_t i = 0; i < this->m_elements.size(); ++i)
				{
					this->m_elements[i] = i + 1;
				}
			}

			// Removes the first element from the free list and returns its index
			DescriptorIndex Allocate()
			{
				assert(this->m_numActiveElements < this->m_elements.size() && "Consider increasing the size of the pool");
				this->m_numActiveElements++;
				DescriptorIndex index = this->m_freeStart;
				this->m_freeStart = this->m_elements[index];
				return index;
			}

			void Release(DescriptorIndex index)
			{
				this->m_numActiveElements--;
				this->m_elements[index] = this->m_freeStart;
				this->m_freeStart = index;
			}

		private:
			std::vector<DescriptorIndex> m_elements;
			DescriptorIndex m_freeStart;
			size_t m_numActiveElements;
		};

	private:
		DescriptorHeapAllocation m_allocation;
		DescriptorIndexPool m_descriptorIndexPool;

	};
}