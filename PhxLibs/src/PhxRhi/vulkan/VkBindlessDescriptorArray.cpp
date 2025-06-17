#include "PhxRhi/PhxRhi_pch.h"
#include "VkBindlessDescriptorArray.h"

#include "VkGfxDevice.h"

using namespace phx::rhi::vk;

void VkBindlessDescriptorArray::Initialize(VkGfxDeviceImpl* device, VkDescriptorType descriptor_type, uint32_t max_slots)
{
	m_device = device;
	m_descriptor_type = descriptor_type;

	m_slot_allocator.Initialize(max_slots);

	VkDevice device = m_device->GetLogicalDevice();
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& props = device->GetDescriptorBufferProperties();

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

    m_buffer = device->CreateBuffer({
        .Usage = Usage::Dynamic,
        .Size = total_size,
        .MiscFlags = ResourceMiscFlags::DescriptorTable,
    });

    // Cache some data
    Buffer_VK* impl = device->GetResourceInternal(m_buffer);
    m_buffer_address = impl->gpu_address;
    m_mapped_data = static_cast<char*>(impl->mapped_data);
}

void phx::rhi::vk::VkBindlessDescriptorArray::Shutdown()
{
    if (m_buffer.IsValid())
        m_device->DeleteBuffer(m_buffer);

    m_buffer = {};
}
