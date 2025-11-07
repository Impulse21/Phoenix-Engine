#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanResourceManager.h"

#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_RHI_WARN(" Pool '" #x "' still contains active handles"); x.Shutdown();


int CreateSubResource(Buffer_VK & buffer, GpuBufferDescriptor const& desc, SubresouceType subresource_type, size_t offset, size_t size = ~0u)
{
    assert(subresource_type == SubresouceType::SRV || subresource_type == SubresouceType::UAV);

    Format format = desc.Format;

    // Is raw buffer
    if (format == Format::UNKNOWN)
    {
        buffer.srv_is_typed = false;
#if false
        // These sections are commented out in the original code, keeping them commented.
        // If uncommented, they would need VkContext:: prefix for m_bindlessStorageBuffers and m_device
        // buffer.srv_index = VkContext::bindless_storage_buffers.Allocate(); // Assuming a new name for this member

        // VkDescriptorBufferInfo buffer_info = {}; // Renamed to snake_case
        // buffer_info.buffer = buffer.vk_buffer;
        // buffer_info.offset = offset;
        // buffer_info.range = size;

        // VkWriteDescriptorSet write = {};
        // write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        // write.dstBinding = 0;
        // write.dstArrayElement = buffer.srv_index;
        // write.descriptorCount = 1;
        // write.dstSet = VkContext::bindless_storage_buffers.DescritporSetVk; // Assuming new name
        // write.pBufferInfo = &buffer_info;

        // vkUpdateDescriptorSets(VkContext::vk_device, 1, &write, 0, nullptr);
#else
        PHX_CORE_WARN("[Vulkan] TODO: Add Bindless support");
#endif
    }
    else
    {
        // Typed buffer
        buffer.srv_is_typed = true;

        VkBufferViewCreateInfo srv_desc = {}; // Renamed to snake_case
        srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        srv_desc.buffer = buffer.vk_buffer;
        srv_desc.flags = 0;
        srv_desc.format = FormatToVkFormat(format);
        srv_desc.offset = offset;
        srv_desc.range = std::min(size, (uint64_t)desc.Size - srv_desc.offset);

        VkResult res = vkCreateBufferView(VkContext::vk_device, &srv_desc, nullptr, &buffer.buffer_view);
        assert(res == VK_SUCCESS);

        if (subresource_type == SubresouceType::SRV)
        {
            // buffer.srv_index = VkContext::bindless_uniform_texel_buffers.Allocate(); // Assuming new name
            if (buffer.buffer_view != VK_NULL_HANDLE)
            {
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                write.dstBinding = 0;
                write.dstArrayElement = buffer.srv_index;
                write.descriptorCount = 1;
                // write.dstSet = VkContext::bindless_uniform_texel_buffers.DescritporSetVk; // Assuming new name
                write.pTexelBufferView = &buffer.buffer_view;
                vkUpdateDescriptorSets(VkContext::vk_device, 1, &write, 0, nullptr);
            }

            return -1;
        }
        else
        {
            // buffer.uav_index = VkContext::bindless_storage_texel_buffers.Allocate(); // Assuming new name
            if (buffer.buffer_view != VK_NULL_HANDLE)
            {
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                write.dstBinding = 0;
                write.dstArrayElement = buffer.uav_index;
                write.descriptorCount = 1;
                // write.dstSet = VkContext::bindless_storage_texel_buffers.DescritporSetVk; // Assuming new name
                write.pTexelBufferView = &buffer.buffer_view;
                vkUpdateDescriptorSets(VkContext::vk_device, 1, &write, 0, nullptr);
            }
            return -1;
        }
    }

    return 0;
}

BufferHandle phx::rhi::VulkanResourceManager::CreateBuffer(const BufferDescriptor& desc, const void* initialData)
{
    PHX_PROFILE_SECTION("Vulkan::PlatformCreateBuffer");
    if (VkContext::vma_allocator == VK_NULL_HANDLE)
        return GpuBufferHandle();

    Handle<GpuBuffer> ret_val = VkContext::buffer_pool.Allocate(); // Renamed to snake_case
    Buffer_VK& impl = *VkContext::buffer_pool.GetHot(ret_val); // Corrected access to buffer_pool

    VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; // Renamed to snake_case
    buffer_info.size = desc.Size;
    buffer_info.usage = 0;

    static const std::vector <std::pair<BindingFlags, VkBufferUsageFlags>> k_usage_mapping = // Renamed to snake_case
    {
        { BindingFlags::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
        { BindingFlags::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
        { BindingFlags::ConstantBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
        { BindingFlags::ShaderResource, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT},
        { BindingFlags::UnorderedAccess, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT},
    };

    for (const auto& [flag, usage_flag] : k_usage_mapping) // Renamed loop variables
    {
        if (phx::EnumHasAnyFlags(desc.BindingFlags, flag))
        {
            buffer_info.usage |= usage_flag;
        }
    }

    // Misc Flags
    static const std::vector <std::pair<ResourceMiscFlags, VkBufferUsageFlags>> k_usage_mapping_misc = // Renamed to snake_case
    {
        { ResourceMiscFlags::BufferRaw, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
        { ResourceMiscFlags::BufferStructured, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
        { ResourceMiscFlags::IndirectArgs, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},
        { ResourceMiscFlags::RayTracing, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR },
        { ResourceMiscFlags::DescriptorTable, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT },
    };

    for (const auto& [flag, usage_flag] : k_usage_mapping_misc) // Renamed loop variables
    {
        if (EnumHasAnyFlags(desc.MiscFlags, flag))
        {
            buffer_info.usage |= usage_flag;
        }
    }

    if (VkContext::vk_features_1_2.bufferDeviceAddress == VK_TRUE)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    buffer_info.flags = 0;

    if (VkContext::queue_gfx.vk_queue_family != VkContext::queue_compute.vk_queue_family || VkContext::queue_compute.vk_queue_family != VkContext::queue_transfer.vk_queue_family)
    {
        buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;

        std::array<uint32_t, 3> families = { VkContext::queue_gfx.vk_queue_family, VkContext::queue_compute.vk_queue_family, VkContext::queue_transfer.vk_queue_family };
        buffer_info.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
        buffer_info.pQueueFamilyIndices = families.data();
        // Note: The original code sets sharingMode to EXCLUSIVE right after CONCURRENT. This might be a bug or intentional override.
        // I'm preserving the original logic, assuming the EXCLUSIVE override is intentional.
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasBuffer))
    {
        // TODO:
    }
    else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Sparse))
    {
        // TODO:
    }
    else
    {
        VmaAllocationCreateInfo alloc_info = {}; // Renamed to snake_case
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

        switch (desc.Usage)
        {
        case Usage::ReadBack:
            alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;

        case Usage::Upload:
            alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;

        case Usage::Dynamic:
            alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;

        case Usage::Default:
        default:
            break;
        }

        if (desc.Alias == nullptr)
        {
            vulkan_check(
                vmaCreateBuffer(VkContext::vma_allocator, &buffer_info, &alloc_info, &impl.vk_buffer, &impl.allocation, nullptr));
        }
        else
        {
            // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
            if (std::holds_alternative<TextureHandle>(desc.Alias->handle))
            {
#if false
                // This section is commented out in the original code, keeping it commented.
                // If uncommented, they would need VkContext:: prefix for m_texturePool and VkResult res.
                // Texture_VK* alias_texture = VkContext::texture_pool.Get(std::get<TextureHandle>(desc.Alias->Handle)); // Renamed to snake_case
                // res = vmaCreateAliasingBuffer2(
                //     VkContext::vma_allocator,
                //     alias_texture->Allocation,
                //     desc.Alias->AliasOffset,
                //     &buffer_info,
                //     &impl.BufferVk);
#else
                PHX_CORE_ASSERT(false, "TODO");
#endif
            }
            else
            {
                Buffer_VK* alias_buffer = VkContext::buffer_pool.GetHot(std::get<GpuBufferHandle>(desc.Alias->handle)); // Renamed to snake_case
                assert(alias_buffer);

                vulkan_check(
                    vmaCreateAliasingBuffer2(
                        VkContext::vma_allocator,
                        alias_buffer->allocation,
                        desc.Alias->offset,
                        &buffer_info,
                        &impl.vk_buffer));

            }
        }

#ifdef PHX_DEBUG
        // Now you have allocInfo.memoryType, which tells you which memory type was used
        VkPhysicalDeviceMemoryProperties memory_properties; // Renamed to snake_case
        vkGetPhysicalDeviceMemoryProperties(VkContext::vk_chosen_physical_device, &memory_properties);

        // Use the memoryTypeIndex to find the memory type
        VkMemoryType memory_type = memory_properties.memoryTypes[impl.allocation->GetMemoryTypeIndex()]; // Renamed to snake_case

        // Find the corresponding heap
        uint32_t heap_index = memory_type.heapIndex; // Renamed to snake_case
        VkMemoryHeap heap = memory_properties.memoryHeaps[heap_index]; // Renamed to snake_case

        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(VkContext::vma_allocator, budgets);

        PHX_CORE_INFO("[Vulkan] Created Buffer on {0} - {1}/{2}", heap_index, budgets[heap_index].usage, heap.size);
#endif
    }

    if (desc.Usage == Usage::ReadBack || desc.Usage == Usage::Upload || desc.Usage == Usage::Dynamic)
    {
        impl.mapped_data = impl.allocation->GetMappedData();
        impl.mapped_data_size = impl.allocation->GetSize();
    }

    if (buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = impl.vk_buffer;
        impl.gpu_address = vkGetBufferDeviceAddress(VkContext::vk_device, &info);
    }

    if (initial_data) // Assuming initial_data is a parameter to this function
    {
        rhi::vk::CopyCtx copy_ctx;
        Buffer_VK* copy_buffer;
        void* mapped_data = nullptr;
        if (desc.Usage == Usage::Upload)
        {
            mapped_data = impl.mapped_data;
        }
        else
        {
            copy_ctx = VkContext::copy_ctx_manager.Allocate(impl.allocation->GetSize());
            copy_buffer = VkContext::buffer_pool.GetHot(copy_ctx.upload_buffer);
            mapped_data = copy_buffer->mapped_data;
        }

        std::memcpy(mapped_data, initial_data, impl.allocation->GetSize());

        if (copy_ctx.IsValid())
        {
            VkBufferCopy copy_region = {}; // Renamed to snake_case
            copy_region.size = desc.Size;
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;

            vkCmdCopyBuffer(
                copy_ctx.transfer_command_buffer,
                copy_buffer->vk_buffer,
                impl.vk_buffer,
                1,
                &copy_region
            );

            VkBufferMemoryBarrier2 barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.buffer = impl.vk_buffer;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            barrier.size = VK_WHOLE_SIZE;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ConstantBuffer))
            {
                barrier.dstAccessMask |= VK_ACCESS_2_UNIFORM_READ_BIT;
            }
            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::VertexBuffer))
            {
                barrier.dstStageMask |= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
                barrier.dstAccessMask |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
            }
            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::IndexBuffer))
            {
                barrier.dstStageMask |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
                barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
            }
            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ShaderResource))
            {
                barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
            }
            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess))
            {
                barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
                barrier.dstAccessMask |= VK_ACCESS_2_SHADER_WRITE_BIT;
            }
            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::IndirectBuffer))
            {
                barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            }
            if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::RayTracing))
            {
                barrier.dstAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            }

            VkDependencyInfo dependency_info = {}; // Renamed to snake_case
            dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency_info.bufferMemoryBarrierCount = 1;
            dependency_info.pBufferMemoryBarriers = &barrier;

            vkCmdPipelineBarrier2(copy_ctx.transition_command_buffer, &dependency_info);

            VkContext::copy_ctx_manager.SubmitAndWait(copy_ctx);
        }
    }

    if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
    {
        CreateSubResource(impl, desc, SubresouceType::SRV, 0u);
    }

    if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
    {
        CreateSubResource(impl, desc, SubresouceType::UAV, 0u);
    }

    return ret_val;
}

void phx::rhi::VulkanResourceManager::DeleteBuffer(BufferHandle handle)
{
    VkContext::EnqueueDelete({
        VkContext::frame_number,
        [handle]()
        {
            Buffer_VK* impl = VkContext::buffer_pool.GetHot(handle);
            // TODO: Move into the deconstructor of struct
            if (impl)
            {
#if false
                // These sections are commented out in the original code, keeping them commented.
                // If uncommented, they would need VkContext:: prefix for the pools and VkContext::vk_device.
                // if (impl->Srv.IsValid())
                // {
                //     if (impl->Srv.IsTyped)
                //     {
                //         VkContext::bindless_uniform_texel_buffers.Free(impl->Srv.Index);
                //     }
                //     else
                //     {
                //         VkContext::bindless_storage_buffers.Free(impl->Srv.Index);
                //     }
                //
                //     if (impl->Srv.ViewVk != VK_NULL_HANDLE)
                //         vkDestroyBufferView(VkContext::vk_device, impl->Uav.ViewVk, nullptr);
                //     impl->Srv = {};
                // }
                // if (impl->Uav.IsValid())
                // {
                //     if (impl->Uav.IsTyped)
                //     {
                //         VkContext::bindless_storage_texel_buffers.Free(impl->Uav.Index);
                //     }
                //     else
                //     {
                //         VkContext::bindless_storage_buffers.Free(impl->Uav.Index);
                //     }
                //
                //     if (impl->Uav.ViewVk != VK_NULL_HANDLE)
                //         vkDestroyBufferView(VkContext::vk_device, impl->Uav.ViewVk, nullptr);
                //     impl->Uav = {};
                // }
#endif
                    // TODO: Descriptors
                    // TODO: Free Views
                    if (impl->buffer_view != VK_NULL_HANDLE)
                        vkDestroyBufferView(VkContext::vk_device, impl->buffer_view, nullptr);

                    vmaDestroyBuffer(VkContext::vma_allocator, impl->vk_buffer, impl->allocation);
                }

                VkContext::buffer_pool.Free(handle);
            }
        });
}
