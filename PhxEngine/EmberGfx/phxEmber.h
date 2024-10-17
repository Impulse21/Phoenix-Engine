#pragma once

#include "phxMemory.h"

#include "phxPlatformDetection.h"
#include "phxGpuDeviceInterface.h"

namespace phx::gfx
{
	using GpuDevice = IGpuDevice;
	using CommandCtx = ICommandCtx;
	namespace EmberGfx
	{
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle);
		void Finalize();

		GpuDevice* GetDevice();
		struct DynamicBuffer
		{
			BufferHandle BufferHandle;
			size_t Offset;
			uint8_t* Data;
		};

		// One per thread - not thread safe
		struct DynamicAllocator
		{
			DynamicAllocator()
			{
				this->Page = GetDevice()->AllocateDynamicMemoryPage(PageSize);
				this->ByteOffset = 0;
			}

			DynamicBuffer Allocate(uint32_t byteSize, uint32_t alignment)
			{
				uint32_t offset = MemoryAlign(ByteOffset, alignment);
				this->ByteOffset = offset + byteSize;

				if (this->ByteOffset > this->PageSize)
				{
					this->Page = GetDevice()->AllocateDynamicMemoryPage(PageSize);
					offset = 0;
					this->ByteOffset = byteSize;
				}

				return DynamicBuffer{
					.BufferHandle = this->Page.BufferHandle,
					.Offset = offset,
					.Data = this->Page.Data + offset
				};
			}

			DynamicMemoryPage Page = {};
			size_t PageSize = 4_MiB;
			uint32_t ByteOffset = 0;
		};
	}
}

