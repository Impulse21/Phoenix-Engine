#include "pch.h"

#include "phx/rhi/TempMemoryBlockAllocator.h"
#include "phx/rhi/GfxDevice.h"

using namespace phx;
using namespace phx::rhi;

void TempMemoryBlockAllocator::Initialize(GfxDevice* device, size_t bufferSize, uint32_t blockSize)
{
	m_gfxDevice = device;
	m_blockSize = blockSize;

	assert((bufferSize& (bufferSize - 1)) == 0);
	this->m_bufferMask = (bufferSize - 1);

	m_buffer = m_gfxDevice->CreateBuffer({
			.DebugName = "Upload Buffer",
			.SizeInBytes = (uint32_t)bufferSize,
			.Usage = Usage::Upload,
		});

	platform::GpuBufferBindings* platformBufferBinding = m_gfxDevice->GetGpuBufferPool().Get<platform::GpuBufferBindings>(m_buffer);
	m_data = reinterpret_cast<uint8_t*>(platformBufferBinding->CpuMappedAddress);

	m_platform.Initialize(device->Platform(), platformBufferBinding);
}

void TempMemoryBlockAllocator::Finalize()
{
	m_platform.Finalize();

	m_data = nullptr;
	m_gfxDevice->DeleteBuffer(m_buffer);
}

void TempMemoryBlockAllocator::EndFrame()
{
	m_platform.EndFrame();
	m_headAtStartOfFrame = m_tail;
}

rhi::DynamicMemoryBlock TempMemoryBlockAllocator::GetNextMemoryBlock()
{
	std::scoped_lock _(this->m_mutex);

	// Checks if the top bits have changes, if so, we need to wrap around.
	if ((m_tail ^ (m_tail + m_blockSize)) & ~m_bufferMask)
	{
		m_tail = (m_tail + m_bufferMask) & ~m_bufferMask;
	}

	if (((m_tail - m_head) + m_blockSize) >= GetBufferSize())
	{
		while (!this->m_inUseRegions.empty())
		{
			auto& region = this->m_inUseRegions.front();
			if (region.Fence->GetCompletedValue() != 1)
			{
				PHX_CORE_WARN("[GPU QUEUE] Stalling waiting for space");
				region.Fence->SetEventOnCompletion(1, NULL);
			}

			region.Fence->Signal(0);
			this->m_availableFences.push_back(region.Fence);

			m_head += region.UsedSize;

			this->m_inUseRegions.pop_front();
		}
	}

	const uint32_t offset = (this->m_tail & m_bufferMask) + m_blockSize;
	m_tail += m_blockSize;

	return DynamicMemoryPage{
		.BufferHandle = this->m_buffer,
		.Offset = offset,
		.Data = reinterpret_cast<uint8_t*>(this->m_data + offset),
	};
}