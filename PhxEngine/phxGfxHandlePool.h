#pragma once

#include <cstring>
#include <utility>
#include <limits>
#include <assert.h>
#include "phxGfxHandle.h"
#include <stdexcept>

#include <iostream>
namespace phx::gfx
{
	template<typename ImplT, typename HT, bool _AllowResize = false>
	class HandlePool
	{
	public:
		HandlePool()
			: m_size(0)
			, m_numActiveEntries(0)
			, m_data(nullptr)
			, m_generations(nullptr)
			, m_freeList(nullptr)
		{
		}

		~HandlePool()
		{
			this->Finalize();
		}

		void Initialize(size_t initCapacity = 16)
		{
			this->m_size = initCapacity;
			this->m_numActiveEntries = 0;

			this->m_data = new ImplT[this->m_size];
			this->m_generations = new uint32_t[this->m_size];
			this->m_freeList = new uint32_t[this->m_size];

			std::memset(this->m_data, 0, this->m_size * sizeof(ImplT));
			std::memset(this->m_generations, 0, this->m_size * sizeof(uint32_t));
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
				for (int i = 0; i < this->m_size; i++)
				{
					this->m_data[i].~ImplT();
				}

				delete[] this->m_data;
				this->m_data = nullptr;
			}

			if (this->m_freeList)
			{
				delete[] this->m_freeList;
				this->m_freeList = nullptr;
			}

			if (this->m_generations)
			{
				delete[] this->m_generations;
				this->m_generations = nullptr;
			}

			this->m_size = 0;
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
			if constexpr !_AllowResize
				throw std::runtime_exception("Ran out of space");

			if (this->m_size == 0)
			{
				this->Initialize(16);
				return;
			}

			size_t newSize = this->m_size * 2;
			auto* newDataArray = new ImplT[newSize];
			auto* newFreeListArray = new uint32_t[newSize];
			auto* newGenerations = new uint32_t[newSize];

			std::memset(newDataArray, 0, newSize * sizeof(ImplT));
			std::memset(newFreeListArray, 0, newSize * sizeof(uint32_t));

			for (size_t i = 0; i < newSize; i++)
			{
				newGenerations[i] = 1;
			}

			// Copy data over
			std::memcpy(newDataArray, this->m_data, this->m_size * sizeof(ImplT));
			// No need to copy the free list as we need to re-populate it.
			// std::memcpy(newFreeListArray, this->m_freeList, this->m_size * sizeof(uint32_t));
			std::memcpy(newGenerations, this->m_generations, this->m_size * sizeof(uint32_t));

			delete[] this->m_data;
			delete[] this->m_freeList;
			delete[] this->m_generations;

			this->m_data = newDataArray;
			this->m_freeList = newFreeListArray;
			this->m_generations = newGenerations;

			this->m_freeListPosition = this->m_size - 1;

			this->m_size = newSize;
			for (size_t i = 0; i < this->m_size - 1; i++)
			{
				this->m_freeList[i] = static_cast<uint32_t>((this->m_size - 1) - i);
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