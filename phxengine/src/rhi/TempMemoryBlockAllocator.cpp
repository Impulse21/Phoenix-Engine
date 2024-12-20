#include "phx/pch.h"

#include "phx/rhi/TempMemoryBlockAllocator.h"
#include "phx/rhi/GfxDevice.h"

using namespace phx;
using namespace phx::rhi;

void TempMemoryBlockAllocator::Initialize(GfxDevice* device, uint32_t bufferSize, uint32_t blockSize)
{
	m_gfxDevice = device;
	m_blockSize = blockSize;

	assert((bufferSize& (bufferSize - 1)) == 0);
	m_bufferMask = (bufferSize - 1);

	m_buffer = m_gfxDevice->CreateBuffer({
			.DebugName = "Upload Buffer",
			.SizeInBytes = (uint32_t)bufferSize,
			.Usage = Usage::Upload,
		});

	platform::GpuBufferBindings* platformBufferBinding = m_gfxDevice->GetGpuBufferPool().Get<platform::GpuBufferBindings>(m_buffer);
	m_data = reinterpret_cast<uint8_t*>(platformBufferBinding->CpuMappedAddress);

	m_platform.Initialize(m_gfxDevice->Platform(), platformBufferBinding);
}


void TempMemoryBlockAllocator::Finalize()
{
	m_platform.Finalize();

	m_data = nullptr;
	m_gfxDevice->DeleteBuffer(m_buffer);
}

void TempMemoryBlockAllocator::EndFrame()
{
	m_platform.EndFrame(m_gfxDevice->Platform(), m_tail - m_headAtStartOfFrame, m_head);
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
		m_platform.WaitForFreeRegions(m_head);
	}

	const uint32_t offset = (this->m_tail & m_bufferMask) + m_blockSize;
	m_tail += m_blockSize;

	return DynamicMemoryBlock{
		.BufferHandle = this->m_buffer,
		.Offset = offset,
		.Data = reinterpret_cast<uint8_t*>(this->m_data + offset),
	};
}