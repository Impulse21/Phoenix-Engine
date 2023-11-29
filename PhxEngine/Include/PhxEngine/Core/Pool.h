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

			this->m_data = this->m_allocator.AllocateArray<ImplT>(this->m_size, 16);
			this->m_generations = this->m_allocator.AllocateArray<uint32_t>(this->m_size, 16);
			this->m_freeList = this->m_allocator.AllocateArray<uint32_t>(this->m_size, 16);

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
				this->m_allocator.Free(this->m_data);
				this->m_data = nullptr;
			}

			if (this->m_freeList)
			{
				this->m_allocator.Free(this->m_freeList);
				this->m_freeList = nullptr;
			}

			if (this->m_generations)
			{
				this->m_allocator.Free(this->m_generations);
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

			// TODO: USE REALLOC INSTREAD for better performance.
			this->m_data = this->m_allocator.RellocateArray<ImplT>(this->m_data, newSize, 16);
			this->m_freeList = this->m_allocator.RellocateArray<uint32_t>(this->m_freeList, newSize, 16);
			this->m_generations = this->m_allocator.RellocateArray<uint32_t>(this->m_generations, newSize, 16);

			this->m_freeListPosition = this->m_size - 1;

			this->m_size = newSize;

			for (size_t i = 0; i < this->m_size - 1; i++)
			{
				this->m_freeList[i] = (this->m_size - 1) - i;
			}
		}

		bool HasSpace() const { return this->m_numActiveEntries < this->m_size; }

	private:
		DefaultAllocator m_allocator;
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