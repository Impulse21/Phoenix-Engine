#include "RHIVulkan.h"


using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

// -- Texture API ---
TextureHandle phx::rhi::CreateTexture(const TextureDescriptor& desc)
{
    TextureHandle ret_val = g_context.pool_textures.Allocate();
    VulkanTexture& impl = *g_context.pool_textures.Get(ret_val);

    impl.vk_format = FormatToVkFormat(desc.format);

    impl.width = desc.width;
    impl.height = desc.height;
    
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .format = impl.vk_format,
        .extent = { .width = desc.width, .height = desc.height, .depth = desc.depth },
        .mipLevels = desc.mip_levels,
        .arrayLayers = desc.array_size,
        .samples = (VkSampleCountFlagBits)desc.sample_count,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = 0,
        // vkCreateImage only accepts UNDEFINED/PREINITIALIZED here — an image
        // can't be "born" already in e.g. COLOR_ATTACHMENT_OPTIMAL. Getting it
        // into desc.initial_state's layout is a separate (currently missing)
        // barrier step; see the disabled transition code further down.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
        image_info.imageType = VK_IMAGE_TYPE_1D;
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
    case TextureType::Texture2DMS:
    case TextureType::Texture2DMSArray:
        image_info.imageType = VK_IMAGE_TYPE_2D;
        break;
    case TextureType::Texture3D:
        image_info.imageType = VK_IMAGE_TYPE_3D;
        break;
    default:
        assert(0);
        break;
    }

    VkResult res = VK_SUCCESS;

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    // TODO: Sparse Textures
    if (EnumHasAnyFlags(desc.misc_flags, ResourceMiscFlags::Sparse))
    {
        PHX_LOG_WARN(phx::Log::Channels::RHI, "Sparse textures are not implemented yet");
    }
    else
    {
        // disable aliasing for now - it requires a lot of work to support it properly, and it's not a priority right now
#if true
        res = vmaCreateImage(
            g_context.vma_allocator,
            &image_info,
            &alloc_info,
            &impl.vk_image,
            &impl.allocation,
            nullptr);
#else
        // TODO: Support image read backs / uploads
        if (!desc.alias.Buffer.IsValid())
        {
            res = vmaCreateImage(
                g_context.vma_allocator,
                &image_info,
                &alloc_info,
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
#endif
        assert(res == VK_SUCCESS);
    }

    PHX_LOG_WARN(
        phx::Log::Channels::RHI,
        "Initializing a texture with data at Creation is not currently supported");

    // Initialize the texture with data is not supported at the moment.
#if false
    if (initial_data)
    {
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
    }
#endif

    bool is_depth = IsFormatDepthSupport(desc.format);

    // -- Create resource views for the texture based on the binding flags ---
    if (EnumHasAnyFlags(desc.binding_flags, BindingFlags::ShaderResource))
    {
        VkImageAspectFlags aspect_mask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageViewCreateInfo view_info = { 
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.texture_type),
            .format = FormatToVkFormat(desc.format),
            .subresourceRange{
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = desc.mip_levels,
                .baseArrayLayer = 0,
                .layerCount = desc.array_size,
            }
        };

        vkCreateImageView(g_context.vk_device, &view_info, nullptr, &impl.vk_view_sampled);

        // unifiedImageLayouts — this texture never leaves GENERAL after its
        // first use (see TransitionToGeneral in VulkanCmdBuffer.cpp), so the
        // baked descriptor and the image's actual layout always agree.
        VkDescriptorImageInfo image_data = {
            .imageView   = impl.vk_view_sampled,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkDescriptorGetInfoEXT descriptor_info = { 
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .data = { .pSampledImage = &image_data }
        };

        impl.srv_index = g_context.descriptor_system.AllocateResource(descriptor_info);
    }

    // --- UAV: Unordered Access View (Storage) ---
    // Characteristics: Mip 0 Only (usually), All Layers, Color aspect only.
    if (EnumHasAnyFlags(desc.binding_flags, BindingFlags::UnorderedAccess))
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.texture_type),
            .format = FormatToVkFormat(desc.format),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = desc.array_size,
            }
        };

        vkCreateImageView(g_context.vk_device, &view_info, nullptr, &impl.vk_view_storage);

        VkDescriptorImageInfo image_data = {
            .imageView = impl.vk_view_storage,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkDescriptorGetInfoEXT descriptor_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .data {.pSampledImage = &image_data }
        };

        impl.uav_index = g_context.descriptor_system.AllocateResource(descriptor_info);
    }

    // --- RTV: Render Target View ---
    // Characteristics: Mip 0 Only, Color aspect. No Bindless index.
    if (EnumHasAnyFlags(desc.binding_flags, BindingFlags::RenderTarget))
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.texture_type),
            .format = FormatToVkFormat(desc.format),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = desc.array_size,
            }
        };

        vkCreateImageView(g_context.vk_device, &view_info, nullptr, &impl.vk_view_rtv);
    }

    // --- DSV: Depth Stencil View ---
    // Characteristics: Mip 0 Only, Depth + Stencil Aspect. No Bindless index.
    if (EnumHasAnyFlags(desc.binding_flags, BindingFlags::DepthStencil))
    {
        const bool has_stencil = IsFormatStencilSupport(desc.format);

        VkImageAspectFlags aspect_mask = has_stencil
            ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            : VK_IMAGE_ASPECT_DEPTH_BIT;

        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = impl.vk_image,
            .viewType = ToVkImageViewType(desc.texture_type),
            .format = FormatToVkFormat(desc.format),
            .subresourceRange = {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = desc.array_size,
            }
        };

        vkCreateImageView(g_context.vk_device, &view_info, nullptr, &impl.vk_view_dsv);
    }

    return ret_val;
}

void phx::rhi::DestroyTexture(TextureHandle handle)
{
#define DESTORY_IMAGE_VIEW(view) if(view != VK_NULL_HANDLE) { vkDestroyImageView(g_context.vk_device, view, nullptr); }
    g_context.deferred_callback_queue.EnqueueDelete({
        .frame = g_context.frame_number, 
        .deferred_func = [handle]() {

			VulkanTexture* impl = g_context.pool_textures.Get(handle);
			// TODO: Move into the deconstructor of struct
			if (!impl)
				return;
            
            DESTORY_IMAGE_VIEW(impl->vk_view_sampled);
            DESTORY_IMAGE_VIEW(impl->vk_view_storage);
            DESTORY_IMAGE_VIEW(impl->vk_view_rtv);
            DESTORY_IMAGE_VIEW(impl->vk_view_dsv);
            
            if (impl->srv_index != rhi::kInvalidDescriptorIndex)
                g_context.descriptor_system.FreeResource(impl->srv_index);
            
            if (impl->uav_index != rhi::kInvalidDescriptorIndex)
                g_context.descriptor_system.FreeResource(impl->uav_index);

            vmaDestroyImage(g_context.vma_allocator, impl->vk_image, impl->allocation);
            g_context.pool_textures.Free(handle);
        }
    });
}

DescriptorIndex phx::rhi::GetShaderResourceIndex(TextureHandle handle)
{
    VulkanTexture* impl = g_context.pool_textures.Get(handle);
    return impl ? impl->srv_index : rhi::kInvalidDescriptorIndex;
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
    PHX_UNUSED(handle);
}

// -- Pipeline State API ---
PipelineStateHandle phx::rhi::CreatePipelineState(const PipelineStateDescriptor& desc)
{
    Handle<PipelineState> ret_val = g_context.pool_pipeline_states.Allocate(); // Renamed to snake_case
    VulkanPipelineState& impl = *g_context.pool_pipeline_states.Get(ret_val); // Corrected access to buffer_pool

    impl.bind_point = ToVkPipelineBindPoint(desc.type);

    VkPipelineShaderStageCreateInfo shader_stages[static_cast<size_t>(ShaderStage::Count)] = {};
    size_t num_stages = 0;

    for (auto& stage_info : desc.shader_stages)
    {
        if (!stage_info.module_handle.IsValid())
            continue;

        VulkanShaderModule& shader_module_impl = *g_context.pool_shader_modules.Get(stage_info.module_handle);

        VkPipelineShaderStageCreateInfo& create_info = shader_stages[num_stages++];
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        create_info.stage = ShaderStageToVulkanShaderStage(stage_info.stage);
        create_info.module = shader_module_impl.vk_shader_module;
        create_info.pName = stage_info.entry_point;
    }

    VkDynamicState dynamic_state_data[] = {
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,

        // Extended Dynamic State 1 & 2 (Requires VK_EXT_extended_dynamic_state features enabled)
        VK_DYNAMIC_STATE_CULL_MODE,
        VK_DYNAMIC_STATE_FRONT_FACE,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
        VK_DYNAMIC_STATE_STENCIL_OP,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(std::size(dynamic_state_data)),
        .pDynamicStates = dynamic_state_data,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = ToVkPrimtivieTopology(desc.prim_type),
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineRasterizationStateCreateInfo raster_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = desc.raster_state.depth_clip_enable ? VK_FALSE : VK_TRUE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL, // ToVkPolygonMode(desc.raster_state.fill_mode),
        .cullMode = VK_CULL_MODE_NONE,// ToVkCullMode(desc.raster_state.cull_mode),
        .frontFace = desc.raster_state.front_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = (desc.raster_state.depth_bias != 0 || desc.raster_state.slope_scaled_depth_bias != 0.0f),
        .depthBiasConstantFactor = static_cast<float>(desc.raster_state.depth_bias),
        .depthBiasClamp = desc.raster_state.depth_bias_clamp,
        .depthBiasSlopeFactor = desc.raster_state.slope_scaled_depth_bias,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = desc.depth_stencil_state.depth_enable,
        .depthWriteEnable = desc.depth_stencil_state.depth_write_mask == DepthWriteMask::All,
        .depthCompareOp = ConvertComparisonFunc(desc.depth_stencil_state.depth_func),
        .depthBoundsTestEnable = desc.depth_stencil_state.depth_bounds_test_enable,
        .stencilTestEnable = desc.depth_stencil_state.stencil_enable,
    };

    if (desc.depth_stencil_state.stencil_enable)
    {
        depth_stencil_ci.front.compareMask = desc.depth_stencil_state.stencil_read_mask;
        depth_stencil_ci.front.writeMask = desc.depth_stencil_state.stencil_write_mask;
        depth_stencil_ci.front.reference = 0; // runtime supplied
        depth_stencil_ci.front.compareOp = ConvertComparisonFunc(desc.depth_stencil_state.front_face.stencil_func);
        depth_stencil_ci.front.passOp = ConvertStencilOp(desc.depth_stencil_state.front_face.stencil_pass_op);
        depth_stencil_ci.front.failOp = ConvertStencilOp(desc.depth_stencil_state.front_face.stencil_fail_op);
        depth_stencil_ci.front.depthFailOp = ConvertStencilOp(desc.depth_stencil_state.front_face.stencil_depth_fail_op);

        depth_stencil_ci.back.compareMask = desc.depth_stencil_state.stencil_read_mask;
        depth_stencil_ci.back.writeMask = desc.depth_stencil_state.stencil_write_mask;
        depth_stencil_ci.back.reference = 0; // runtime supplied
        depth_stencil_ci.back.compareOp = ConvertComparisonFunc(desc.depth_stencil_state.back_face.stencil_func);
        depth_stencil_ci.back.passOp = ConvertStencilOp(desc.depth_stencil_state.back_face.stencil_pass_op);
        depth_stencil_ci.back.failOp = ConvertStencilOp(desc.depth_stencil_state.back_face.stencil_fail_op);
        depth_stencil_ci.back.depthFailOp = ConvertStencilOp(desc.depth_stencil_state.back_face.stencil_depth_fail_op);
    }

    VkPipelineColorBlendAttachmentState blend_attachments[8];
    uint32_t valid_attachment_count = 0;

    for (uint32_t i = 0; i < 8; ++i) // Assuming max 8 attachments
    {
        const auto& target = desc.blend_state.targets[i];
        if (i >= desc.render_pass_info.color_attachments.size())
            break;

        auto& att = blend_attachments[valid_attachment_count++];
        att.blendEnable = target.blend_enable;
        att.srcColorBlendFactor = ConvertBlendValue(target.src_blend);
        att.dstColorBlendFactor = ConvertBlendValue(target.dest_blend);
        att.colorBlendOp = ConvertBlendOp(target.blend_op);
        att.srcAlphaBlendFactor = ConvertBlendValue(target.src_blend_alpha);
        att.dstAlphaBlendFactor = ConvertBlendValue(target.dest_blend_alpha);
        att.alphaBlendOp = ConvertBlendOp(target.blend_op_alpha);
        att.colorWriteMask = ToVkColorComponentFlags(target.color_write_mask);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = valid_attachment_count,
        .pAttachments = blend_attachments,
    };

    VkFormat color_formats[8];
    for (uint32_t i = 0; i < desc.render_pass_info.color_attachments.size(); ++i)
    {
        color_formats[i] = FormatToVkFormat(desc.render_pass_info.color_attachments[i]);
    }

    VkFormat ds_format = FormatToVkFormat(desc.render_pass_info.depth_stencil_format);
    auto IsStencilFormat = [](VkFormat fmt) {
        return fmt == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            fmt == VK_FORMAT_D24_UNORM_S8_UINT ||
            fmt == VK_FORMAT_S8_UINT;
        };

    VkPipelineRenderingCreateInfo rendering_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(desc.render_pass_info.color_attachments.Size()),
        .pColorAttachmentFormats = color_formats,
        .depthAttachmentFormat = ds_format,
        .stencilAttachmentFormat = IsStencilFormat(ds_format) ? ds_format : VK_FORMAT_UNDEFINED,
    };

    VkPipelineViewportStateCreateInfo viewport_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 0,
        .scissorCount = 0,
    };

    VkGraphicsPipelineCreateInfo pipeline_ci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_ci,
        .flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
        .stageCount = static_cast<uint32_t>(num_stages),
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pViewportState = &viewport_ci,
        .pRasterizationState = &raster_ci,
        .pMultisampleState = &multisample_ci,
        .pDepthStencilState = &depth_stencil_ci,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = g_context.descriptor_system.pipeline_layout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
    };

    vulkan_check(
        vkCreateGraphicsPipelines(
            g_context.vk_device,
            g_context.vk_pipeline_cache,
            1,
            &pipeline_ci,
            nullptr,
            &impl.vk_pipeline));

    // These are declared dynamic state above (VK_DYNAMIC_STATE_CULL_MODE etc.)
    // — the static values baked into raster_ci/depth_stencil_ci are ignored
    // at draw time. BindPipelineState reads these cached values instead.
    impl.depth_test_enable = desc.depth_stencil_state.depth_enable;
    impl.depth_write_enable = desc.depth_stencil_state.depth_write_mask == DepthWriteMask::All;
    impl.depth_compare_op = desc.depth_stencil_state.depth_func;
    impl.cull_mode = desc.raster_state.cull_mode;
    impl.front_counter_clockwise = desc.raster_state.front_counter_clockwise;

    return ret_val;
}

void phx::rhi::DestroyPipelineState(PipelineStateHandle handle)
{
    g_context.deferred_callback_queue.EnqueueDelete({
        .frame = g_context.frame_number, 
        .deferred_func = [handle]() {

            VulkanPipelineState* impl = g_context.pool_pipeline_states.Get(handle);
            if (impl)
            {
                vkDestroyPipeline(g_context.vk_device, impl->vk_pipeline, nullptr);

                g_context.pool_pipeline_states.Free(handle);
            }
        }
    });
}

// -- Shader Module API ---
ShaderModuleHandle phx::rhi::CreateShaderModule(const ShaderModuleDescriptor& desc)
{
    ShaderModuleHandle ret_val = g_context.pool_shader_modules.Allocate();
    VulkanShaderModule& impl = * g_context.pool_shader_modules.Get(ret_val);

    VkShaderModuleCreateInfo vk_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc.byte_code.Size() * sizeof(uint32_t), // VkShaderModuleCreateInfo::codeSize is in bytes; Span::Size() is word count
        .pCode = desc.byte_code.begin(),
    };

    vulkan_check(
        vkCreateShaderModule(g_context.vk_device, &vk_module_info, nullptr, &impl.vk_shader_module)
    );

    return ret_val;
}

void phx::rhi::DestroyShaderModule(ShaderModuleHandle handle)
{
    VulkanShaderModule* impl = g_context.pool_shader_modules.Get(handle);
    vkDestroyShaderModule(g_context.vk_device, impl->vk_shader_module, nullptr);
    g_context.pool_shader_modules.Free(handle);
}