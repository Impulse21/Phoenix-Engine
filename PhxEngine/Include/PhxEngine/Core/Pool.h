#pragma once

#include <PhxEngine/Core/Memory.h>

#include <cstring>
#include <utility>
#include <limits>
#include <assert.h>
#include "Handle.h"

#include <iostream>
namespace PhxEngine::Core
{
	template<typename ImplT, typename HT>
	class Pool
	{
	public:
		Pool()
			: m_size(0)
			, m_numActiveEntries(0)
		{
		}

		~Pool()
		{
			this->Finalize();
		}

		void Initialize(size_t initCapacity)
		{
			this->m_size = initCapacity;
			this->m_numActiveEntries = 0;

			this->m_data = phx_new_arr(ImplT, this->m_size);
			this->m_generations = phx_new_arr(uint32_t, this->m_size);
			this->m_freeList = phx_new_arr(uint32_t, this->m_size);

			std::memset(this->m_data, 0, this->m_size * sizeof(ImplT));
			std::memset(this->m_freeList, 0, this->m_size * sizeof(uint32_t));

			for (size_t i = 0; i < this->m_size; i++)
			{
				this->m_generations[i] = 1;
			}

			this->m_freeListPosition = this->m_size - 1;
			for (size_t i = 0; i < this->m_size; i++)
			{
				this->m_freeList[i] = static_cast<uint32_t>((this->m_size - 1) - i);
			}
		}

		void Finalize()
		{
			if (this->m_data)
			{
				phx_delete_arr(this->m_data);
				this->m_data = nullptr;
			}

			if (this->m_freeList)
			{
				phx_delete_arr(this->m_freeList);
				this->m_freeList = nullptr;
			}

			if (this->m_generations)
			{
				phx_delete_arr(this->m_generations);
				this->m_generations = nullptr;
			}
		}

		ImplT* Get(Handle<HT> handle)
		{
			if (!this->Contains(handle))
			{
				return nullptr;
			}

			return this->m_data + handle.m_index;
		}

		bool Contains(Handle<HT> handle) const
		{
			return
				handle.IsValid() &&
				handle.m_index < this->m_size &&
				this->m_generations[handle.m_index] == handle.m_generation;
		}


		template<typename... Args>
		Handle<HT> Emplace(Args&&... args)
		{
			if (!this->HasSpace())
			{
				this->Resize();
			}

			Handle<HT> handle;
			// Get a free index
			handle.m_index = this->m_freeList[this->m_freeListPosition];
			handle.m_generation = this->m_generations[handle.m_index];
			this->m_freeListPosition--;
			this->m_numActiveEntries++;

			new (this->m_data + handle.m_index) ImplT(std::forward<Args>(args)...);

			return handle;
		}

		Handle<HT> Insert(ImplT const& Data)
		{
			return this->Emplace(Data);
		}

		void Release(Handle<HT> handle)
		{
			if (!this->Contains(handle))
			{
				return;
			}

			this->Get(handle)->~ImplT();
			new (this->m_data + handle.m_index) ImplT();
			this->m_generations[handle.m_index] += 1;

			// To prevent the risk of re assignment, block index for being allocated
			if (this->m_generations[handle.m_index] == std::numeric_limits<uint32_t>::max())
			{
				return;
			}

			this->m_freeList[++this->m_freeListPosition] = handle.m_index;
			this->m_numActiveEntries--;
		}

		bool IsEmpty() const { return this->m_numActiveEntries == 0; }

	private:
		void Resize()
		{
			if (this->m_size == 0)
			{
				this->Initialize(16);
				return;
			}

			size_t newSize = this->m_size * 2;

			auto* newDataArray = phx_new_arr(ImplT, newSize);
			auto* newFreeListArray = phx_new_arr(uint32_t, newSize);
			auto* newGenerations = phx_new_arr(uint32_t, newSize);

			std::memset(newDataArray, 0, newSize * sizeof(ImplT));
			std::memset(newFreeListArray, 0, newSize * sizeof(uint32_t));
			std::memset(newGenerations, 0, newSize * sizeof(uint32_t));

			// Copy data over
			std::memcpy(newDataArray, this->m_data, this->m_size * sizeof(ImplT));
			// No need to copy the free list as we need to re-populate it.
			// std::memcpy(newFreeListArray, this->m_freeList, this->m_size * sizeof(uint32_t));
			std::memcpy(newGenerations, this->m_generations, this->m_size * sizeof(uint32_t));

			phx_delete_arr(this->m_data);
			phx_delete_arr(this->m_freeList);
			phx_delete_arr(this->m_generations);

			this->m_data = newDataArray;
			this->m_freeList = newFreeListArray;
			this->m_generations = newGenerations;

			this->m_freeListPosition = this->m_size - 1;

			this->m_size = newSize;

			for (size_t i = 0; i < this->m_size - 1; i++)
			{
				this->m_freeList[i] = (this->m_size - 1) - i;
			}
		}

		bool HasSpace() const { return this->m_numActiveEntries < this->m_size; }

	private:
		size_t m_size;
		size_t m_numActiveEntries;
		size_t m_freeListPosition;

		// free array
		uint32_t* m_freeList;

		// Generation array
		uint32_t* m_generations;

		// Data Array
		ImplT* m_data;
	};
}