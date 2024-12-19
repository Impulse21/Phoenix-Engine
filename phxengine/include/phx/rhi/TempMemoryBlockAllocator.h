#pragma once

#include "phx/core/Memory.h"
#include "phx/core/Log.h"

#include "phx/rhi/RHITypes.h"
#include "RHIPlatformTypes.h"

namespace phx::rhi
{
	class GfxDevice;

	struct DynamicMemoryBlock
	{
		GpuBufferHandle BufferHandle;
		size_t Offset;
		uint8_t* Data;
	};

	class TempMemoryBlockAllocator
	{
	public:
		void Initialize(GfxDevice* device, size_t bufferSize, uint32_t blockSize = 4_MiB);
		void Finalize();

		void EndFrame();

		DynamicMemoryBlock GetNextMemoryBlock();

		uint32_t GetBufferSize() { return (this->m_bufferMask + 1); }
		uint32_t GetBlockSize() { return m_blockSize; }

	private:
		uint32_t m_blockSize;
		platform::TempMemoryBlockAllocator m_platform;

		GfxDevice* m_gfxDevice;

		GpuBufferHandle m_buffer;
		uint32_t m_bufferMask;
		uint32_t m_headAtStartOfFrame = 0;
		uint32_t m_head = 0;
		uint32_t m_tail = 0;

		uint8_t* m_data;

		std::mutex m_mutex;
	};
}