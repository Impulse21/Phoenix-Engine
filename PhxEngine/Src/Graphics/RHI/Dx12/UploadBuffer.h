#pragma once


#include "PhxEngine/Graphics/RHI/PhxRHI.h"
#include "Common.h"

#include <deque>
#include <memory>

#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)

namespace PhxEngine::RHI::Dx12
{
	class GraphicsDevice;
	class UploadBuffer
	{
	public:
		struct Allocation
		{
			void* CpuData;
			D3D12_GPU_VIRTUAL_ADDRESS Gpu;
			BufferHandle BufferHandle;
			size_t Offset;
		};

		explicit UploadBuffer(GraphicsDevice& device, size_t pageSize = MB(10));

		Allocation Allocate(size_t sizeInBytes, size_t alignment);

		void Reset();

		size_t GetPageSize() { this->m_pageSize; }

	private:
		struct Page
		{
			Page(GraphicsDevice& device, size_t sizeInBytes);
			~Page();

			bool HasSpace(size_t sizeInBytes, size_t alignment) const;

			Allocation Allocate(size_t sizeInBytes, size_t aligmnet);

			void Reset();

		private:
			GraphicsDevice& m_device;
			BufferHandle m_buffer;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

			size_t m_pageSize;
			size_t m_offset;
		};

	private:
		using PagePool = std::deque<std::shared_ptr<Page>>;
		std::shared_ptr<Page> RequestPage();

	private:
		GraphicsDevice& m_device;
		PagePool m_pagePool;
		PagePool m_availablePages;

		std::shared_ptr<Page> m_currentPage;

		size_t m_pageSize;
	};
}
