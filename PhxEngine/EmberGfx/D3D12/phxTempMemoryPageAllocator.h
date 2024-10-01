#pragma once

#include "pch.h"
#include "TempMemoryAllocator.h"

namespace phx::gfx::d3d12
{
	class TempMemoryPageAllocatorD3D12 final : public TempMemoryPageAllocator
	{
	public:
		TempMemoryPageAllocatorD3D12(size_t bufferSize, size_t frameCount);
		TempMemoryPage GetNextMemoryPage(size_t size) = 0;

		void EndFrame();

	private:
		HandleOwner<Buffer> m_uploadBuffer;
		struct FrameMarker
		{
			size_t StartOffset = 0;
			size_t EndOffset = 0;
			Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		};
		std::vector<FrameMarker> m_fences;
	};
}