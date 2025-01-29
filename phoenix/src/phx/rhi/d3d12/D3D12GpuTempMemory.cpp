#include "phxpch.h"
#include "D3D12GpuTempMemory.h"

#include "D3D12Core.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;

void TempMemoryBlockAllocator::Initialize(uint32_t bufferSize, uint32_t blockSize)
{
	m_blockSize = blockSize;

	assert((bufferSize & (bufferSize - 1)) == 0);
	m_bufferMask = (bufferSize - 1);

	m_buffer = g_bufferPool->CreateBuffer({
			.DebugName = "Upload Buffer",
			.SizeInBytes = (uint32_t)bufferSize,
			.Usage = Usage::Upload,
		});

}
