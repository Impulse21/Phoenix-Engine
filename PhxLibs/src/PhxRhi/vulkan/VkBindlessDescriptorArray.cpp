#include "PhxRhi/PhxRhi_pch.h"

#include <PhxRhi/PhxRhi.h>

#include "VkRhi_Internal.h"
#include "VkBindlessDescriptorArray.h"

using namespace phx::RHI::vk;

void VkBindlessDescriptorArray::Initialize(VkGfxDeviceImpl* device, VkDescriptorType descriptor_type, uint32_t max_slots)
{
	m_device = device;
	m_descriptor_type = descriptor_type;

	m_slot_allocator.Initialize(max_slots);

	// VkDevice vk_logical_device = m_device->GetLogicalDevice();
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& props = RHI::VkContext::vk_descriptor_buffer_properties;

    switch (descriptor_type)
    {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
        m_descriptor_size = props.samplerDescriptorSize;
        break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        m_descriptor_size = props.combinedImageSamplerDescriptorSize;
        break;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        m_descriptor_size = props.sampledImageDescriptorSize;
        break;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        m_descriptor_size = props.storageImageDescriptorSize;
        break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        m_descriptor_size = props.uniformBufferDescriptorSize;
        break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        m_descriptor_size = props.storageBufferDescriptorSize;
        break;
    default:
        PHX_CORE_ASSERT(false, "Unsupported Type")
        break;
    }

    PHX_CORE_ASSERT(m_descriptor_size != 0, "Unsupported Type");

    const uint32_t total_size = m_descriptor_size * max_slots;

    m_buffer = RHI::CreateBuffer({
        .Size = total_size,
        .Usage = RHI::Usage::Dynamic,
        .MiscFlags = RHI::ResourceMiscFlags::DescriptorTable,
    });

    // Cache some data
    RHI::Buffer_VK* impl = RHI::VkContext::buffer_pool.GetHot(m_buffer);
    m_buffer_address = impl->gpu_address;
    m_mapped_data = static_cast<char*>(impl->mapped_data);
}

void phx::RHI::vk::VkBindlessDescriptorArray::Shutdown()
{
    if (m_buffer.IsValid())
        RHI::DeleteBuffer(m_buffer);

    m_buffer = {};
}
