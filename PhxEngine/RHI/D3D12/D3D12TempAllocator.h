#pragma once

#include <D3D12Common.h>
#include <RHI/RHIResources.h>
#include <Core/Memory.h>
#include <memory>

#include <deque>

namespace PhxEngine::RHI::D3D12
{

	struct TempAllocation
	{
		void* Data;
		size_t ByteOffset;
		BufferHandle BufferHandle;
		D3D12_GPU_VIRTUAL_ADDRESS Gpu;
	};

	class D3D12TempAllocator
	{
	public:
		void Initialize(size_t pageSize = PhxMB(100));
		void Finalize();

		TempAllocation Allocate(size_t sizeInBytes, size_t alignment);

		void Reset();

		size_t GetPageSize() { return this->m_pageSize; }

	private:
		struct Page
		{
			Page(size_t sizeInBytes);
			~Page();
			bool HasSpace(size_t sizeInBytes, size_t alignment) const;
			TempAllocation Allocate(size_t sizeInBytes, size_t alignment);

			void Reset();

		private:
			BufferHandle m_buffer;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

			size_t m_pageSize;
			size_t m_offset;
		};

	private:
		using PagePool = std::deque<std::shared_ptr<Page>>;
		std::shared_ptr<Page> RequestPage();

	private:
		PagePool m_pagePool;
		PagePool m_availablePages;

		std::shared_ptr<Page> m_currentPage;

		size_t m_pageSize;
	};

}

