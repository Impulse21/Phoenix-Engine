#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanDevice.h"

phx::rhi::VulkanDevice::VulkanDevice()
	: m_resource_manager(this)
	, m_gpu_memory_allocator(this)
{
}

bool phx::rhi::VulkanDevice::Initialize(Descriptor const& desc)
{

	return false;
}
