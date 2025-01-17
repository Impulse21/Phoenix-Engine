#pragma once

#include <type_traits>
#include <cstring>
#include <utility>
#include <limits>
#include <assert.h>
#include <stdexcept>
#include <iostream>
#include <variant>

#include "phx/rhi/Handle.h"
#include "phx/core/Memory.h"

namespace phx::rhi
{
	constexpr size_t kPageSize = 4_MiB;
	template <typename>
	inline constexpr bool always_false = false;

	template<class THandle, class TDataHot, class TDataCold = std::monostate>
	class ResourcePool
	{
		static_assert(std::is_default_constructible_v<TDataHot>);
		static_assert(std::is_default_constructible_v<TDataCold>, "TDataHot should have a trivial destructor");

	public:
		ResourcePool(uint16_t maxHandles)
			: m_maxEntries(std::min(maxHandles, std::numeric_limits<uint16_t>::max()))
			, m_freeListHead(0)
			, m_commitedIndices(0)
		{
			m_dataHot = static_cast<TDataHot*>(phx::VirtualMemReserve(m_maxEntries * sizeof(TDataHot)));
			m_dataCold = static_cast<TDataCold*>(phx::VirtualMemReserve(m_maxEntries * sizeof(TDataCold)));

			m_freeList = static_cast<uint16_t*>(phx::VirtualMemReserve(m_maxEntries * sizeof(uint16_t)));
			m_generations = static_cast<uint16_t*>(phx::VirtualMemReserve(m_maxEntries * sizeof(uint16_t)));

			if (!m_dataHot || !m_freeList || !m_generations)
				throw std::runtime_error("Failed to reserve virtual pool memory.");

			m_indicesPerPageHot			=	kPageSize / sizeof(TDataHot);
			m_indicesPerPageCold		=	kPageSize / sizeof(TDataCold);
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
			if (m_dataHot)
			{
				for (size_t i = 0; i < m_commitedIndices; i++)
				{
					m_dataHot[i].~TDataHot();
				}

				phx::VirtualMemFree(m_dataHot);
				m_dataHot = nullptr;
			}

			if (m_dataCold)
			{
				for (size_t i = 0; i < m_commitedIndices; i++)
				{
					m_dataCold[i].~TDataCold();
				}

				phx::VirtualMemFree(m_dataCold);
				m_dataCold = nullptr;
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

		Handle<THandle> Allocate()
		{
			if (m_freeListHead >= m_maxEntries)
				throw std::runtime_error("Pool is out of memory!");

			if (m_freeListHead >= m_commitedIndices)
				CommitPages();


			Handle<THandle> handle;
			// Get a free index
			handle.m_index = m_freeList[m_freeListHead++];
			handle.m_generation = m_generations[handle.m_index];

			new (this->m_dataHot + handle.m_index) TDataHot();
			new (this->m_dataCold + handle.m_index) TDataCold();

			return handle;
		}

		void Free(Handle<THandle> handle)
		{
			if (!Contains(handle))
			{
				return;
			}

			GetHot(handle)->~TDataHot();
			if constexpr (!std::is_trivially_destructible_v<TDataCold>)
				GetCold(handle)->~TDataCold();

			m_dataHot[handle.m_index] = {};

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
			if constexpr (std::is_same_v<T, TDataHot>)
			{
				return GetHot(handle);
			}
			else if constexpr (std::is_same_v<T, TDataCold>)
			{
				return GetCold(handle);
			}
			else
			{
				static_assert(always_false<T>, "Unsupported handle type!");
			}
		}

		template<typename T>
		const T* Get(Handle<THandle> handle) const
		{
			if constexpr (std::is_same_v<T, TDataHot>)
			{
				return GetHot(handle);
			}
			else if constexpr (std::is_same_v<T, TDataCold>)
			{
				return GetCold(handle);
			}
			else
			{
				static_assert(always_false<T>, "Unsupported handle type!");
			}
		}

		TDataHot* GetHot(Handle<THandle> handle)
		{
			if (!Contains(handle))
			{
				return nullptr;
			}

			return m_dataHot + handle.m_index;
		}

		const TDataHot* GetHot(Handle<THandle> handle) const
		{
			if (!Contains(handle))
			{
				return nullptr;
			}

			return m_dataHot + handle.m_index;
		}

		TDataCold* GetCold(Handle<THandle> handle)
		{
			if (!Contains(handle))
			{
				return nullptr;
			}

			return m_dataCold + handle.m_index;
		}

		const TDataCold* GetCold(Handle<THandle> handle) const
		{
			if (!Contains(handle))
			{
				return nullptr;
			}

			return m_dataCold + handle.m_index;
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

			phx::VirtualMemCommit(reinterpret_cast<char*>(m_dataHot) + m_commitedIndices * sizeof(TDataHot), commitSize);
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
		TDataHot*		m_dataHot;
		TDataCold*		m_dataCold;

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
