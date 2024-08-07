#pragma once

#include <memory>
#include <deque>
#include "D3D12Common.h"
#include "Core/phxMemory.h"

namespace phx::rhi::d3d12
{
    class D3D12GfxDevice;

	struct UploadAllocation
	{
		void* CpuData;
		D3D12_GPU_VIRTUAL_ADDRESS Gpu;
		BufferHandle BufferHandle;
		size_t Offset;
	};

	class UploadBufferAllocator
	{
	public:
		class Page
		{
		public:
			~Page();
			bool HasSpace(size_t sizeInBytes, size_t alignment) const;
			UploadAllocation Allocate(size_t sizeInBytes, size_t alignment);

		private:
			Page(D3D12GfxDevice* device, size_t sizeInBytes);
			void Reset();

		private:
			D3D12GfxDevice* m_gfxDevice;
			BufferHandle m_buffer;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

			size_t m_pageSize;
			size_t m_offset;
		};

	public:
		void Initialize(D3D12GfxDevice* device, size_t pageSize = 100_MiB);

		std::shared_ptr<Page> RequestPage();
		UploadAllocation Allocate(size_t sizeInBytes, size_t alignment);

		void Reset();

		size_t GetPageSize() { return this->m_pageSize; }


	private:
		using PagePool = std::deque<std::shared_ptr<Page>>;

	private:
		D3D12GfxDevice* m_gfxDevice;
		PagePool m_pagePool;
		PagePool m_availablePages;

		std::shared_ptr<Page> m_currentPage;

		size_t m_pageSize;
	};

	class UploadBuffer
	{
	public:
		void Initialize(D3D12GfxDevice* device, size_t pageSize = 100_MiB);

		UploadAllocation Allocate(size_t sizeInBytes, size_t alignment);

		void Reset();

		size_t GetPageSize() { return this->m_pageSize; }

	private:
		struct Page
		{
			Page(D3D12GfxDevice* device, size_t sizeInBytes);
			~Page();
			bool HasSpace(size_t sizeInBytes, size_t alignment) const;
			UploadAllocation Allocate(size_t sizeInBytes, size_t alignment);

			void Reset();

		private:
			D3D12GfxDevice* m_gfxDevice;
			BufferHandle m_buffer;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

			size_t m_pageSize;
			size_t m_offset;
		};

	private:
		using PagePool = std::deque<std::shared_ptr<Page>>;
		std::shared_ptr<Page> RequestPage();

	private:
		D3D12GfxDevice* m_gfxDevice;
		PagePool m_pagePool;
		PagePool m_availablePages;

		std::shared_ptr<Page> m_currentPage;

		size_t m_pageSize;
	};

}