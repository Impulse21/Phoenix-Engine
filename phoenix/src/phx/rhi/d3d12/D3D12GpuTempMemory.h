#pragma once

#include <deque>
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
			D3D12_GPU_VIRTUAL_ADDRESS GpuAddress = {};
			uint8_t* Data = nullptr;

			bool IsValid() const { return Data == nullptr; }
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
		void TempMemoryBlockAllocator::WaitForFreeRegions(uint32_t& head);

	private:
		uint32_t m_blockSize;
		uint32_t m_bufferMask;
		uint32_t m_headAtStartOfFrame = 0;
		uint32_t m_head = 0;
		uint32_t m_tail = 0;

		uint8_t* m_data;
		std::mutex m_mutex;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Fence>> m_fencePool;
		std::deque<ID3D12Fence*> m_availableFences;
		struct UsedRegion
		{
			uint32_t UsedSize = 0;
			ID3D12Fence* Fence;
		};
		std::deque<UsedRegion> m_inUseRegions;
	};

	struct TempAllocator
	{
		[[nodiscard]] TempBuffer Allocate(uint32_t byteSize, uint32_t alignment)
		{
			TempMemoryBlockAllocator* blockAllocator = TempMemoryBlockAllocator::Ptr;
			const uint32_t blockSize = static_cast<uint32_t>(blockAllocator->GetBlockSize());
			PHX_ASSERT(byteSize <= blockSize);

			uint32_t offset = AlignUp(ByteOffset, alignment);
			ByteOffset = offset + byteSize;
			
			if (!CurrentBlock.IsValid() || ByteOffset > blockSize)
			{
				CurrentBlock = TempMemoryBlockAllocator::Ptr->GetNextMemoryBlock();
				offset = 0;
				ByteOffset = byteSize;
			}

			return TempBuffer{
				.ByteOffset = offset,
				.Data = CurrentBlock.Data + offset,
				.GpuAddress = CurrentBlock.GpuAddress + offset,
			};
		}

		void Reset()
		{
			ByteOffset = 0;
			CurrentBlock = {};

		}

		TempMemoryBlockAllocator::TempMemoryBlock CurrentBlock;
		uint32_t ByteOffset = 0;
	};
}