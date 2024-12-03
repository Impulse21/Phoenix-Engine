#pragma once

#include <type_traits>
#include <cstring>
#include <utility>
#include <limits>
#include <assert.h>
#include <stdexcept>
#include <iostream>

#include "phx/rhi/Handle.h"
#include "phx/core/Memory.h"

namespace phx::rhi
{
	constexpr size_t kCacheLineSize = 8 * sizeof(uint64_t);
	constexpr size_t kPageSize = 4_MiB;
	template <typename>
	inline constexpr bool always_false = false;

	template<class THandle, class TData>
	class ResourcePool
	{
	public:
		ResourcePool(uint16_t maxHandles)
			: m_maxEntries(std::min(maxHandles, std::numeric_limits<uint16_t>::max()))
			, m_freeListHead(0)
			, m_commitedIndices(0)
		{
			m_data = static_cast<TData*>(phx::VirtualMemReserve(m_maxEntries * sizeof(TData));

			m_freeList = static_cast<uint16_t*>(phx::VirtualMemReserve(m_maxEntries * sizeof(uint16_t)));
			m_generations = static_cast<uint16_t*>(phx::VirtualMemReserve(m_maxEntries * sizeof(uint16_t)));

			if (!m_data || !m_freeList || !m_generations)
				throw std::runtime_error("Failed to reserve virtual pool memory.");

			m_indicesPerPageHot			=	kPageSize / sizeof(TData);
			m_indicesPerPageCold		=	kPageSize / sizeof(TColdData);
			m_indicesPerPageMetadata	=	kPageSize / sizeof(uint16_t);

			m_indicesPerCommit = std::min({ m_indicesPerPageHot, m_indicesPerPageCold, m_indicesPerPageMetadata });

			CommitPages();  // Commit an initial 4 pages
		}

		~ResourcePool()
		{
			Finalize();
		}

		void Finalize()
		{
			if (m_data)
			{
				for (int i = 0; i < m_commitedIndices; i++)
				{
					m_data[i].~TData();
				}

				phx::VirtualMemFree(m_data);
				m_data = nullptr;
			}

			if (m_freeList)
			{
				phx::VirtualMemFree(m_freeList);
				m_freeList = nullptr;
			}

			if (m_generations)
			{
				phx::VirtualMemFree(m_generations);
				m_generations = nullptr;
			}
		}

		template<typename... Args>
		Handle<THandle> Emplace(Args&&... args)
		{
			if (m_freeListHead >= m_maxEntries)
				throw std::runtime_error("Pool is out of memory!");

			if (m_freeListHead >= m_commitedIndices)
				CommitPages();


			Handle<THandle> handle;
			// Get a free index
			handle.m_index = m_freeList[m_freeListHead++];
			handle.m_generation = m_generations[handle.m_index];

			new (this->m_data + handle.m_index) TData(std::forward<Args>(args)...);

			return handle;
		}

		Handle<THandle> Insert(TData const& data)
		{
			return this->Emplace(data);
		}

		void Free(Handle<THandle> handle)
		{
			if (!Contains(handle))
			{
				return;
			}

			GetHot(handle)->~TData();
			GetCold(handle)->~TColdData();

			m_data[handle.m_index] = {};

			m_generations[handle.m_index]++;

			// To prevent the risk of re assignment, block index for being allocated
			if (m_generations[handle.m_index] == std::numeric_limits<uint16_t>::max())
			{
				return;
			}

			m_freeList[--m_freeListHead] = handle.m_index;
		}

		template<typename T>
		T* Get(Handle<THandle> handle)
		{
			if constexpr (std::is_same_v<T, TData>)
			{
				return GetHot(handle);
			}
			else if constexpr (std::is_same_v<T, TColdData>)
			{
				return GetCold(handle);
			}
			else
			{
				static_assert(always_false<T>, "Unsupported handle type!");
			}
		}

		TData* Get(Handle<THandle> handle)
		{
			if (!Contains(handle))
			{
				return nullptr;
			}

			return m_data + handle.m_index;
		}

		const TData* Get(Handle<THandle> handle) const
		{
			if (!Contains(handle))
			{
				return nullptr;
			}

			return m_data + handle.m_index;
		}

		bool Contains(Handle<THandle> handle) const
		{
			return
				handle.IsValid() &&
				handle.m_index < m_commitedIndices &&
				m_generations[handle.m_index] == handle.m_generation;
		}

		bool IsEmpty() const { return m_freeListHead == 0; }

	private:
		void CommitPages(size_t pagesToCommit = 1)
		{
			size_t commitSize = pagesToCommit * kPageSize;

			phx::VirtualMemCommit(reinterpret_cast<char*>(m_data) + m_commitedIndices * sizeof(TData), commitSize);
			phx::VirtualMemCommit(reinterpret_cast<char*>(m_freeList) + m_commitedIndices * sizeof(uint16_t), commitSize);
			phx::VirtualMemCommit(reinterpret_cast<char*>(m_generations) + m_commitedIndices * sizeof(uint16_t), commitSize);

			const size_t previousCommitedIndices = m_commitedIndices;
			m_commitedIndices += pagesToCommit * m_indicesPerCommit;

			// Initialize the free list with available indices
			for (size_t i = previousCommitedIndices; i < m_commitedIndices; ++i)
			{
				m_freeList[i] = (uint16_t)i;
				m_generations[i] = 0;
			}
		}

	private:
		const size_t	m_maxEntries;
		TData*		m_data;
		TColdData*		m_dataCold;

		uint16_t* m_freeList;
		uint16_t* m_generations;
		size_t	m_freeListHead;

		size_t m_commitedIndices;
		size_t m_indicesPerPageHot;
		size_t m_indicesPerPageCold;
		size_t m_indicesPerPageMetadata;
		size_t m_indicesPerCommit;
	};
}