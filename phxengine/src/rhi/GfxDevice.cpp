#include "phx/phx_pch.h"

#include "phx/rhi/GfxDevice.h"
#include "phx/core/ThreadPool.h"

using namespace phx::rhi;

void GfxDevice::Initialize()
{

}

void GfxDevice::Finalize()
{
}

GfxDevice::GfxDevice(rhi::GfxDeviceDescriptor const& descriptor)
	: m_platformDevice(descriptor)
	, m_texturePool(100)
	, m_gpuBufferPool(100)
	, m_pipelineStatePool(100)
	, m_swapChainPool(1)
	, m_commandListPool(static_cast<uint16_t>(ThreadPool::GetNumCores() * 2)) // Double for async
{
	m_blockAllocator.Initialize(this, 128_MiB, 4_MiB);
}

GfxDevice::~GfxDevice()
{
	m_blockAllocator.Finalize();
}