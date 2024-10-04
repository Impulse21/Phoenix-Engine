#pragma once

#include <deque>

namespace phx::gfx
{
	struct DynamicMemoryPage
	{
		BufferHandle BufferHandle;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
		uint8_t* Data;
	};

	class GpuRingAllocator
	{
	public:
		void Initialize(size_t bufferSize);
		void Finalize();

		void EndFrame(ID3D12CommandQueue* q);

		DynamicMemoryPage Allocate(uint32_t allocSize);

		uint32_t GetBufferSize() { return (this->m_bufferMask + 1); }

	private:
		BufferHandle m_buffer;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress = 0;
		uint32_t m_bufferMask;
		uint32_t m_headAtStartOfFrame = 0;
		uint32_t m_head = 0;
		uint32_t m_tail = 0;

		uint8_t* m_data;

		std::mutex m_mutex;

		std::vector<Microsoft::WRL::ComPtr<ID3D12Fence>> m_fencePool;
		std::deque<ID3D12Fence*> m_availableFences;
		struct UsedRegion
		{
			uint32_t UsedSize = 0;
			ID3D12Fence* Fence;
		};
		std::deque<UsedRegion> m_inUseRegions;
	};

}

