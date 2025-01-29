#pragma once

#include <phx/core/Assert.h>
#include <phx/core/Memory.h>

#include "D3D12Base.h"

#include "phx/rhi/RHITypes.h"

namespace phx::rhi::d3d12
{
	struct TempBuffer
	{
		size_t ByteOffset;
		uint8_t* Data;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
	};

	class TempMemoryBlockAllocator
	{
	public:
		inline static TempMemoryBlockAllocator* Ptr = nullptr;

		struct TempMemoryBlock
		{
			D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
			uint8_t* data;
		};

		// -- Main interface method ---
	public:
		[[nodiscard]] TempMemoryBlock GetNextMemoryBlock();

	public:
		void Initialize(uint32_t bufferSize, uint32_t blockSize = 4_MiB);
		void Finalize();

		void EndFrame();


		uint32_t GetBufferSize() { return (this->m_bufferMask + 1); }
		uint32_t GetBlockSize() { return m_blockSize; }

	private:
		uint32_t m_blockSize;

		GpuBufferHandle m_buffer;
		uint32_t m_bufferMask;
		uint32_t m_headAtStartOfFrame = 0;
		uint32_t m_head = 0;
		uint32_t m_tail = 0;

		uint8_t* m_data;
		std::mutex m_mutex;
	};

	struct TempAllocator
	{
		[[nodiscard]] TempBuffer Allocate(uint32_t byteSize, uint32_t alignment)
		{
			TempMemoryBlockAllocator* blockAllocator = TempMemoryBlockAllocator::Ptr;
			const size_t blockSize = blockAllocator->GetBlockSize();
			PHX_ASSERT(byteSize <= blockSize);

			uint32_t offset = AlignUp(ByteOffset, alignment);
			ByteOffset = offset + byteSize;

			//
			if ()


		}

		void Reset()
		{
			ByteOffset = 0;

		}

		Temp
		uint32_t ByteOffset = 0;
	};
}