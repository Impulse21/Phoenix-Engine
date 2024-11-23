#include "pch.h"

#include "phx/rhi/GfxDevice.h"

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
{

}

GfxDevice::~GfxDevice()
{

}