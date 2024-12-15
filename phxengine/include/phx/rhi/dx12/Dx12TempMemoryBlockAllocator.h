#pragma once

#include "Dx12Common.h"
#include "Dx12GfxDevice.h"

#include <vector>

namespace phx::rhi::dx12
{

	class TempMemoryBlockAllocator
	{
	public:
		void Initialize(GfxDeviceDx12& device, dx12::GpuBufferBindings* bindings);
		void Finalize();

		void EndFrame(GfxDeviceDx12& device);

	private:
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