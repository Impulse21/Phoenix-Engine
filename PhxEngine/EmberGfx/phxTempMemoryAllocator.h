#pragma once

#include "phxGfxDeviceResources.h"
#include "phxHandle.h"
#include "phxMemory.h"

namespace phx::gfx
{
	struct TempBuffer
	{
		Handle<Buffer> Buffer;
		uint32_t GpuOffset;
		uint8_t* Data;
	};

	class TempMemoryPageAllocator
	{
	public:
		struct TempMemoryPage
		{
			Handle<Buffer> Buffer;
			uint32_t GpuOffset;
			uint8_t Data;
		};

		virtual TempMemoryPage GetNextMemoryPage(size_t size) = 0;
		virtual ~TempMemoryPageAllocator() = default;
	};

	struct TempAllocator
	{
		TempBuffer Allocate(size_t byteSize, size_t alignment = 16)
		{
			uint32_t offset = MemoryAlign(this->ByteOffset, alignment);
			this->ByteOffset = offset + byteSize;

			if (this-ByteOffset > PageSize)
			{
				this->CurrentPage = PageAllocator->GetNextMemoryPage(PageSize);
				offset = 0;
				this->ByteOffset = 0;
			}

			return TempBuffer{
				.Buffer = CurrentPage.Buffer,
				.GpuOffset = CurrentPage.GpuOffset + offset,
				.Data = CurrentPage.Data + offset
			};
		}

		TempMemoryPageAllocator* PageAllocator = nullptr;
		TempMemoryPageAllocator::TempMemoryPage CurrentPage;
		uint32_t PageSize = 4_MiB;
		uint32_t ByteOffset;
	};
}
