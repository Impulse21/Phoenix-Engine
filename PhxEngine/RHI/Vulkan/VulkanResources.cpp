#include "RHIVulkan.h"


using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

CommandBufferHandle phx::rhi::CreateCommandBuffer(const CommandBufferDesc& desc) 
{
    vulkan::CommandBufferImpl* impl = {};
    CommandBufferHandle cmd_handle = g_context.pool_cmd_buffer.Allocate(impl);
 
    // Providied by begin command buffer
    impl->queue_type = desc.type;
    impl->cmd_buffer = VK_NULL_HANDLE;

    return cmd_handle;
}

void phx::rhi::DestoryCommandBuffer(CommandBufferHandle handle) 
{
    // Command Queue doesn't own anything - maybe check if it's open
    // and recording?
    if (!handle.IsValid())
        return;

    CommandBufferImpl* cmd_impl = g_context.pool_cmd_buffer.Get(handle);
    PHX_ASSERT(cmd_impl->cmd_buffer == VK_NULL_HANDLE);

    g_context.pool_cmd_buffer.Free(handle);
}


// -- Texture API ---
TextureHandle phx::rhi::CreateTexture(const TextureDescriptor& desc)
{
    TextureHandle ret_val = g_context.pool_textures.Allocate();
    VulkanTexture& impl = *g_context.pool_textures.Get(ret_val);

    impl.vk_format = FormatToVkFormat(desc.format);

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .extent = { .width = desc.width, .height = desc.height, .depth = desc.depth },
        .format = impl.vk_format,
        .arrayLayers = desc.array_size,
        .mipLevels = desc.mip_levels,
        .samples = (VkSampleCountFlagBits)desc.sample_count,
        .initialLayout = ResourceStateToImageLayout(desc.initial_state),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = 0
    };

    static const std::vector <std::pair<BindingFlags, VkImageUsageFlags>> k_usage_mapping =
    {
        { BindingFlags::ShaderResource, VK_IMAGE_USAGE_SAMPLED_BIT},
        { BindingFlags::UnorderedAccess, VK_IMAGE_USAGE_STORAGE_BIT},
        { BindingFlags::RenderTarget, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
        { BindingFlags::DepthStencil, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
        { BindingFlags::ShadingRate, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR},
    };

    // Build up usage flags based on the binding flags
    for (const auto& [flag, usageFlag] : k_usage_mapping)
    {
        if (EnumHasAnyFlags(desc.binding_flags, flag))
        {
            image_info.usage |= usageFlag;
        }
    }

    if (EnumHasAnyFlags(desc.binding_flags, BindingFlags::UnorderedAccess))
    {
        if (IsFormatSRGB(desc.format))
        {
            image_info.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
        }
    }

    // Build  up usage flags based on the misc flags
    static const std::vector <std::pair<ResourceMiscFlags, VkImageUsageFlags>> k_usage_mapping_misc =
    {
        { ResourceMiscFlags::TransientAttachment, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT},
        { ResourceMiscFlags::TypedFormatCasting, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT},
        { ResourceMiscFlags::TypelessFormatCasting, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT},
    };

    for (const auto& [flag, usageFlag] : k_usage_mapping_misc)
    {
        if (EnumHasAnyFlags(desc.misc_flags, flag))
        {
            image_info.usage |= usageFlag;
        }
    }
    
    if (desc.texture_type == TextureType::TextureCube || desc.texture_type == TextureType::TextureCubeArray)
    {
        image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    if (!EnumHasAnyFlags(desc.misc_flags, ResourceMiscFlags::TransientAttachment))
    {
        image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    const bool are_seperate_queues = 
        g_context.queue_family_indices.HasAsyncCompute() || 
        g_context.queue_family_indices.HasAsyncTransfer();

    if (are_seperate_queues)
    {
        image_info.sharingMode = VK_SHARING_MODE_CONCURRENT;

        u32 num_queues = 1;
        if (g_context.queue_family_indices.HasAsyncCompute())
            num_queues++;
        if (g_context.queue_family_indices.HasAsyncTransfer())
            num_queues++; 

        std::array<u32, 3> queue_families;
        queue_families[0] = g_context.queue_family_indices.graphics_family.value();

        if (g_context.queue_family_indices.HasAsyncCompute())
        {
            queue_families[1] = g_context.queue_family_indices.async_compute_family.value();
        }
        if (g_context.queue_family_indices.HasAsyncTransfer())
        {
            queue_families[2] = g_context.queue_family_indices.async_transfer_family.value();
        }
        
        image_info.queueFamilyIndexCount = num_queues;
        image_info.pQueueFamilyIndices = queue_families.data();
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    switch (desc.texture_type)
    {
    case TextureType::Texture1D:
    case TextureType::Texture1DArray:
        imageInfo.imageType = VK_IMAGE_TYPE_1D;
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
    case TextureType::Texture2DMS:
    case TextureType::Texture2DMSArray:
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        break;
    case TextureType::Texture3D:
        imageInfo.imageType = VK_IMAGE_TYPE_3D;
        break;
    default:
        assert(0);
        break;
    }

    VkResult res = VK_SUCCESS;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // TODO: Sparse Textures
    if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Sparse))
    {
        PHX_RHI_WARN("Sparse textures are not implemented yet");
    }
    else
    {
        // TODO: Support image read backs / uploads
        if (!desc.Alias.Buffer.IsValid())
        {
            res = vmaCreateImage(
                g_vulkan.vma_allocator,
                &imageInfo,
                &allocInfo,
                &impl.vk_image,
                &impl.allocation,
                nullptr);
        }
        else
        {
#if false
            // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
            if (std::holds_alternative<TextureHandle>(desc.Alias->Handle))
            {
                Texture_VK* aliasTexture = m_texturePool.Get(std::get<TextureHandle>(desc.Alias->Handle));
                res = vmaCreateAliasingImage2(
                    m_vmaAllocator,
                    aliasTexture->Allocation,
                    desc.Alias->AliasOffset,
                    &imageInfo,
                    &impl.ImageVk);
            }
            else
            {
                Buffer_VK* aliasBuffer = m_bufferPool.Get(std::get<BufferHandle>(desc.Alias->Handle));
                assert(aliasBuffer);
                res = vmaCreateAliasingImage2(
                    m_vmaAllocator,
                    aliasBuffer->Allocation,
                    desc.Alias->AliasOffset,
                    &imageInfo,
                    &impl.ImageVk);
            }
#else
            PHX_RHI_ERROR("Texture Aliasing is not supported yet");
#endif
        }

        assert(res == VK_SUCCESS);
    }

    if (initial_data)
    {
        PHX_RHI_WARN("Initializing a texture with data at Creation is not currently supported");
#if false
        CopyCtxManager::Ctx ctx = m_copyCtxManager.Begin(impl.Allocation->GetSize());
        void* mappedData = ctx.MappedData;

        std::vector<VkBufferImageCopy> copyRegions;

        VkDeviceSize copyOffset = 0;
        uint32_t initDataIdx = 0;
        for (uint32_t layer = 0; layer < desc.ArraySize; ++layer)
        {
            uint32_t width = imageInfo.extent.width;
            uint32_t height = imageInfo.extent.height;
            uint32_t depth = imageInfo.extent.depth;
            for (uint32_t mip = 0; mip < desc.MipLevels; mip++)
            {
                const SubresourceData& subresourceData = initData[initDataIdx++];
                const uint32_t blockSize = GetFormatBlockSize(desc.Format);
                const uint32_t numBlocksX = std::max(1u, width / blockSize);
                const uint32_t numBlocksY = std::max(1u, height / blockSize);
                const uint32_t dstRowPitch = numBlocksX * GetFormatStride(desc.Format);
                const uint32_t dstSlicePitch = dstRowPitch * numBlocksY;
                const uint32_t srcRowPitch = subresourceData.rowPitch;
                const uint32_t srcSlicePitch = subresourceData.slicePitch;
                for (uint32_t z = 0; z < depth; ++z)
                {
                    uint8_t* dstSlice = (uint8_t*)mappedData + copyOffset + dstSlicePitch * z;
                    uint8_t* srcSlice = (uint8_t*)subresourceData.pData + srcSlicePitch * z;
                    for (uint32_t y = 0; y < numBlocksY; ++y)
                    {
                        std::memcpy(
                            dstSlice + dstRowPitch * y,
                            srcSlice + srcRowPitch * y,
                            dstRowPitch);
                    }
                }

                assert(ctx.IsValid());
                VkBufferImageCopy copyRegion = {};
                copyRegion.bufferOffset = copyOffset;
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;

                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = mip;
                copyRegion.imageSubresource.baseArrayLayer = layer;
                copyRegion.imageSubresource.layerCount = 1;

                copyRegion.imageOffset = { 0, 0, 0 };
                copyRegion.imageExtent = {
                    width,
                    height,
                    depth };

                copyRegions.push_back(copyRegion);

                copyOffset += dstSlicePitch * depth;

                // fix for validation: on transfer queue the srcOffset must be 4-byte aligned
                copyOffset = MemoryAlign(copyOffset, VkDeviceSize(4));

                width = std::max(1u, width / 2);
                height = std::max(1u, height / 2);
                depth = std::max(1u, depth / 2);
            }
        }

        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = impl.ImageVk;
        barrier.oldLayout = imageInfo.initialLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(ctx.TransferCommandBuffer, &dependencyInfo);

        Buffer_VK* staggingBuffer = m_bufferPool.Get(ctx.UploadBuffer);
        vkCmdCopyBufferToImage(
            ctx.TransferCommandBuffer,
            staggingBuffer->BufferVk,
            impl.ImageVk,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            (uint32_t)copyRegions.size(),
            copyRegions.data()
        );

        std::swap(barrier.srcStageMask, barrier.dstStageMask);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = ConvertImageLayout(desc.InitialState);
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = _ParseResourceState(desc.InitialState);
        vkCmdPipelineBarrier2(ctx.TransferCommandBuffer, &dependencyInfo);

        m_copyCtxManager.Submit(ctx);
#endif
    }

    bool is_depth = IsFormatDepthSupport(desc.Format);

    if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ShaderResource))
    {
        VkImageAspectFlags aspect_mask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageViewCreateInfo view_info = { 
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.Type),
            .format = FormatToVkFormat(desc.Format),
            .subresourceRange{
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = desc.MipLevels,
                .baseArrayLayer = 0,
                .layerCount = desc.ArraySize,
            }
        };

        vkCreateImageView(g_vulkan.vk_device, &view_info, nullptr, &impl.vk_view_sampled);

        VkDescriptorImageInfo image_data = {
            .imageView = impl.vk_view_sampled,
            .imageLayout = is_depth
                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorGetInfoEXT descriptor_info = { 
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .data = { .pSampledImage = &image_data }
        };

        impl.srv_index = g_vulkan.descriptor_system.AllocateResource(descriptor_info);
    }

    // --- UAV: Unordered Access View (Storage) ---
    // Characteristics: Mip 0 Only (usually), All Layers, Color aspect only.
    if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess))
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.Type),
            .format = FormatToVkFormat(desc.Format),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = desc.ArraySize,
            }
        };

        vkCreateImageView(g_vulkan.vk_device, &view_info, nullptr, &impl.vk_view_storage);

        VkDescriptorImageInfo image_data = {
            .imageView = impl.vk_view_storage,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkDescriptorGetInfoEXT descriptor_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .data {.pSampledImage = &image_data }
        };

        impl.uav_index = g_vulkan.descriptor_system.AllocateResource(descriptor_info);
    }

    // --- RTV: Render Target View ---
    // Characteristics: Mip 0 Only, Color aspect. No Bindless index.
    if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::RenderTarget))
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.Type),
            .format = FormatToVkFormat(desc.Format),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = desc.ArraySize,
            }
        };

        vkCreateImageView(g_vulkan.vk_device, &view_info, nullptr, &impl.vk_view_rtv);
    }

    // --- DSV: Depth Stencil View ---
    // Characteristics: Mip 0 Only, Depth + Stencil Aspect. No Bindless index.
    if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::DepthStencil))
    {
        const bool has_stencil = IsFormatStencilSupport(desc.Format);

        VkImageAspectFlags aspect_mask = has_stencil
            ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            : VK_IMAGE_ASPECT_DEPTH_BIT;

        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.Type),
            .format = FormatToVkFormat(desc.Format),
            .subresourceRange = {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = desc.ArraySize,
            }
        };

        vkCreateImageView(g_vulkan.vk_device, &view_info, nullptr, &impl.vk_view_dsv);
    }

    return retVal;
}

void phx::rhi::DestroyTexture(TextureHandle handle)
{
    
}

// -- Buffer API ---
GpuBufferHandle phx::rhi::CreateBuffer(const GpuBufferDescriptor& desc)
{
    
    return {};
}

void phx::rhi::DestroyBuffer(GpuBufferHandle handle)
{

}

// -- Sampler API ---
SamplerHandle phx::rhi::CreateSampler(const SamplerDescriptor& desc)
{
    PHX_UNUSED(desc);
    PHX_ASSERT(false);
    return {};
}

void phx::rhi::DestroySampler(SamplerHandle handle)
{
}

// -- Pipeline State API ---
PipelineStateHandle phx::rhi::CreatePipelineState(const PipelineStateDescriptor& desc)
{
    return {};
}

void phx::rhi::DestroyPipelineState(PipelineStateHandle handle)
{
}

// -- Shader Module API ---
ShaderModuleHandle phx::rhi::CreateShaderModule(const ShaderModuleDescriptor& desc)
{
    return {};
}

void phx::rhi::DestroyShaderModule(ShaderModuleHandle handle)
{
}