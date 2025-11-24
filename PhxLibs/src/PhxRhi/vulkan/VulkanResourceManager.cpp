#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanResourceManager.h"

#include "VulkanBackend.h"
#include "VulkanGpuAllocator.h"

#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_RHI_WARN(" Pool '" #x "' still contains active handles"); x.Shutdown();


using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr size_t kMaxNumSwapchains = 1;
    constexpr size_t kMaxPipelineStates = 200;
    constexpr size_t kMaxNumBuffers = 4096;
    //constexpr size_t kMaxNumTextures = 4096;
}

phx::rhi::VulkanResourceManager::VulkanResourceManager(VulkanBackend* vulkan_backend, VulkanGpuAllocator* vulkan_allocator)
    : vulkan_backend(vulkan_backend)
    , vulkan_allocator(vulkan_allocator)
{
}

bool VulkanResourceManager::Initialize()
{
    swapchain_pool.Initialize(kMaxNumSwapchains);
    buffer_pool.Initialize(kMaxNumBuffers);
    pipeline_state_pool.Initialize(kMaxPipelineStates);
    shader_module_pool.Initialize(kMaxPipelineStates * 2);

    VkPipelineCacheCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    vulkan_check(
        vkCreatePipelineCache(vulkan_backend->vk_device, &createInfo, nullptr, &vk_pipeline_cache));

    return true;
}

void phx::rhi::VulkanResourceManager::Shutdown()
{
    LOG_AND_SHUTDOWN_POOL(buffer_pool);
    LOG_AND_SHUTDOWN_POOL(pipeline_state_pool);
    LOG_AND_SHUTDOWN_POOL(shader_module_pool);
    LOG_AND_SHUTDOWN_POOL(swapchain_pool);

    vkDestroyPipelineCache(vulkan_backend->vk_device, vk_pipeline_cache, nullptr);
}

SwapchainHandle phx::rhi::VulkanResourceManager::CreateSwapchain(const SwapchainDesc& desc)
{
    PHX_PROFILE_SECTION("Vulkan::CreateSwapchain");
    vkb::SwapchainBuilder swapchain_builder(
        vulkan_backend->vk_chosen_physical_device,
        vulkan_backend->vk_device,
        vulkan_backend->vk_surface); 

    auto swap_ret = swapchain_builder
        .set_desired_format({ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(desc.Width, desc.Height)
        .set_old_swapchain(VK_NULL_HANDLE) // For initial creation
        .build();

    if (!swap_ret)
    {
        PHX_RHI_ERROR("Failed to create swapchain: {0}", swap_ret.error().message());
        // This is a critical failure during init
        return {};
    }

    Handle<Swapchain> ret_val           = swapchain_pool.Allocate();
    VulkanSwapchainFrame& impl_frame    = *swapchain_pool.GetHot(ret_val);
    VulkanSwapchain& impl               = *swapchain_pool.GetCold(ret_val);

    vkb::Swapchain vkb_swapchain = swap_ret.value(); // Renamed to snake_case
    impl.vk_swapchain = vkb_swapchain.swapchain;
    impl_frame.vk_swapchain_extent = vkb_swapchain.extent;
    impl_frame.vk_swapchain = impl.vk_swapchain;

    const uint32_t num_images = vkb_swapchain.image_count;
    impl.vk_swapchain_image_format = vkb_swapchain.image_format;
    impl.vk_images.resize(num_images);
    impl.vk_image_views.resize(num_images);
    impl.vk_render_finished_sem.resize(num_images);

    // We only need to aquire for the number inflight.
    impl.vk_image_available_sem.resize(cMaxInflightFrames);

    auto swapchain_images = vkb_swapchain.get_images().value();
    auto swapchain_image_views = vkb_swapchain.get_image_views().value();

    for (size_t i = 0; i < num_images; i++)
    {
        impl.vk_images[i] = swapchain_images[i];
        impl.vk_image_views[i] = swapchain_image_views[i];
    }

    VkSemaphoreCreateInfo swapchain_sem_info = {};
    swapchain_sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t i = 0; i < impl.vk_render_finished_sem.size(); ++i)
    {
        VkResult result = vkCreateSemaphore(
            vulkan_backend->vk_device,
            &swapchain_sem_info,
            nullptr,
            &impl.vk_render_finished_sem[i]);

        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create queue swapchain semaphores");
    }

    for (size_t i = 0; i < impl.vk_image_available_sem.size(); ++i)
    {
        // Create the semaphore and store it in the array
        VkResult result = vkCreateSemaphore(
            vulkan_backend->vk_device,
            &swapchain_sem_info,
            nullptr,
            &impl.vk_image_available_sem[i]);

            if (result != VK_SUCCESS)
                PHX_RHI_ERROR("Failed to create queue swapchain semaphores");
    }

    PHX_RHI_INFO(
        "Swapchain Initialized. Extent: {0}x{1}, Format: {2}, Images: {3}",
        impl_frame.vk_swapchain_extent.width,
        impl_frame.vk_swapchain_extent.height,
        "", // Placeholder for format name conversion
        num_images);

    return ret_val;
}

void phx::rhi::VulkanResourceManager::DeleteSwapchain(SwapchainHandle handle)
{
    VulkanSwapchain* impl = swapchain_pool.GetCold(handle);
    for (auto image_view : impl->vk_image_views) // Renamed to snake_case
    {
        if (image_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(vulkan_backend->vk_device, image_view, nullptr);
        }
    }

    if (impl->vk_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(vulkan_backend->vk_device, impl->vk_swapchain, nullptr);
        impl->vk_swapchain = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < impl->vk_image_available_sem.size(); ++i)
    {
        vkDestroySemaphore(vulkan_backend->vk_device, impl->vk_image_available_sem[i], nullptr);
    }

    for (size_t i = 0; i < impl->vk_render_finished_sem.size(); ++i)
    {
        vkDestroySemaphore(vulkan_backend->vk_device, impl->vk_render_finished_sem[i], nullptr);
    }
    swapchain_pool.Free(handle);
}

TextureHandle phx::rhi::VulkanResourceManager::GetSwapchainBackBuffer(SwapchainHandle /*handle*/)
{
    PHX_RHI_WARN("Unable to get back buffer at the moment");
    return {};
}

void phx::rhi::VulkanResourceManager::ResizeSwapchain(SwapchainHandle /*handle*/, uint32_t /*width*/, uint32_t /*height*/)
{
}

void VulkanResourceManager::RunGarbageCollection(uint64_t completed_frame)
{
    deferred_delete_queue.Flush(completed_frame);
}

BufferHandle VulkanResourceManager::CreateBuffer(const BufferDescriptor& desc, const void* initial_data)
{
    PHX_PROFILE_SECTION("Vulkan::PlatformCreateBuffer");

    Handle<Buffer> ret_val = buffer_pool.Allocate(); // Renamed to snake_case
    VulkanBuffer& impl = *buffer_pool.GetHot(ret_val); // Corrected access to buffer_pool

    VkBufferCreateInfo buffer_info = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; // Renamed to snake_case
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

    if (vulkan_backend->vk_features_1_2.bufferDeviceAddress == VK_TRUE)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    buffer_info.flags = 0;

    VmaAllocationInfo vma_alloc_info;

    if (vulkan_backend->queues[CommandQueueType::Graphics].vk_queue_family != vulkan_backend->queues[CommandQueueType::Compute].vk_queue_family ||
        vulkan_backend->queues[CommandQueueType::Compute].vk_queue_family != vulkan_backend->queues[CommandQueueType::Copy].vk_queue_family)
    {
        buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;

        std::array<uint32_t, 3> families = 
        { 
            vulkan_backend->queues[CommandQueueType::Graphics].vk_queue_family,
            vulkan_backend->queues[CommandQueueType::Compute].vk_queue_family,
            vulkan_backend->queues[CommandQueueType::Copy].vk_queue_family
        };

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
        PHX_RHI_WARN("Alias Buffers are not implemented yet");
    }
    else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Sparse))
    {
        PHX_RHI_WARN("Sparse Buffers are not implemented yet");
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
                vmaCreateBuffer(vulkan_allocator->vma_allocator, &buffer_info, &alloc_info, &impl.vk_buffer, &impl.allocation, nullptr));
            vmaGetAllocationInfo(vulkan_allocator->vma_allocator, impl.allocation, &vma_alloc_info);
        }
        else
        {
            // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
            if (std::holds_alternative<TextureHandle>(desc.Alias->handle))
            {
#if false
                // This section is commented out in the original code, keeping it commented.
                // If uncommented, they would need  prefix for m_texturePool and VkResult res.
                // Texture_VK* alias_texture = texture_pool.Get(std::get<TextureHandle>(desc.Alias->Handle)); // Renamed to snake_case
                // res = vmaCreateAliasingBuffer2(
                //     vma_allocator,
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
                VulkanBuffer* alias_buffer = buffer_pool.GetHot(std::get<BufferHandle>(desc.Alias->handle)); // Renamed to snake_case
                assert(alias_buffer);

                vulkan_check(
                    vmaCreateAliasingBuffer2(
                        vulkan_allocator->vma_allocator,
                        alias_buffer->allocation,
                        desc.Alias->offset,
                        &buffer_info,
                        &impl.vk_buffer));

            }
        }

#ifdef PHX_DEBUG
        // Now you have allocInfo.memoryType, which tells you which memory type was used
        VkPhysicalDeviceMemoryProperties memory_properties; // Renamed to snake_case
        vkGetPhysicalDeviceMemoryProperties(vulkan_backend->vk_chosen_physical_device, &memory_properties);


        VkMemoryType memory_type = memory_properties.memoryTypes[vma_alloc_info.memoryType]; // Renamed to snake_case

        // Find the corresponding heap
        uint32_t heap_index = memory_type.heapIndex; // Renamed to snake_case
        VkMemoryHeap heap = memory_properties.memoryHeaps[heap_index]; // Renamed to snake_case

        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(vulkan_allocator->vma_allocator, budgets);

        PHX_CORE_INFO("[Vulkan] Created Buffer on {0} - {1}/{2}", heap_index, budgets[heap_index].usage, heap.size);
#endif
    }

    if (desc.Usage == Usage::ReadBack || desc.Usage == Usage::Upload || desc.Usage == Usage::Dynamic)
    {
        impl.mapped_data = vma_alloc_info.pMappedData;
        impl.mapped_data_size = vma_alloc_info.size;
    }

    if (buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = impl.vk_buffer;
        impl.gpu_address = vkGetBufferDeviceAddress(vulkan_backend->vk_device, &info);
    }

    if (initial_data) // Assuming initial_data is a parameter to this function
    {
        PHX_RHI_WARN("Initializing a buffer with data at start up is not currently supported");
#if false
        rhi::vk::CopyCtx copy_ctx;
        Buffer_VK* copy_buffer;
        void* mapped_data = nullptr;
        if (desc.Usage == Usage::Upload)
        {
            mapped_data = impl.mapped_data;
        }
        else
        {
            copy_ctx = copy_ctx_manager.Allocate(impl.allocation->GetSize());
            copy_buffer = buffer_pool.GetHot(copy_ctx.upload_buffer);
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

            copy_ctx_manager.SubmitAndWait(copy_ctx);
        }
#endif
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

void VulkanResourceManager::DeleteBuffer(BufferHandle handle)
{
    deferred_delete_queue.EnqueueDelete({
        frame_number,
        [=, this]() { DeleteBufferImmediate(handle); }
    });
}

uint64_t phx::rhi::VulkanResourceManager::GetGpuAddress(BufferHandle handle)
{
    VulkanBuffer* impl = buffer_pool.GetHot(handle);
    return impl->gpu_address;
}

void phx::rhi::VulkanResourceManager::DeleteBufferImmediate(BufferHandle handle)
{
	VulkanBuffer* impl = buffer_pool.GetHot(handle);
	// TODO: Move into the deconstructor of struct
	if (impl)
	{
#if false
        // These sections are commented out in the original code, keeping them commented.
        // If uncommented, they would need  prefix for the pools and vk_device.
        // if (impl->Srv.IsValid())
        // {
        //     if (impl->Srv.IsTyped)
        //     {
        //         bindless_uniform_texel_buffers.Free(impl->Srv.Index);
        //     }
        //     else
        //     {
        //         bindless_storage_buffers.Free(impl->Srv.Index);
        //     }
        //
        //     if (impl->Srv.ViewVk != VK_NULL_HANDLE)
        //         vkDestroyBufferView(vk_device, impl->Uav.ViewVk, nullptr);
        //     impl->Srv = {};
        // }
        // if (impl->Uav.IsValid())
        // {
        //     if (impl->Uav.IsTyped)
        //     {
        //         bindless_storage_texel_buffers.Free(impl->Uav.Index);
        //     }
        //     else
        //     {
        //         bindless_storage_buffers.Free(impl->Uav.Index);
        //     }
        //
        //     if (impl->Uav.ViewVk != VK_NULL_HANDLE)
        //         vkDestroyBufferView(vk_device, impl->Uav.ViewVk, nullptr);
        //     impl->Uav = {};
        // }
#endif
                    // TODO: Descriptors
                    // TODO: Free Views
        if (impl->buffer_view != VK_NULL_HANDLE)
            vkDestroyBufferView(vulkan_backend->vk_device, impl->buffer_view, nullptr);

        vmaDestroyBuffer(vulkan_allocator->vma_allocator, impl->vk_buffer, impl->allocation);
	}

	buffer_pool.Free(handle);
}


TextureHandle phx::rhi::VulkanResourceManager::CreateTexture(const TextureDescriptor& /*desc*/, const void* /*initial_data*/)
{
    return TextureHandle();
}

void phx::rhi::VulkanResourceManager::DeleteTexture(TextureHandle /*handle*/)
{
}

ShaderModuleHandle phx::rhi::VulkanResourceManager::CreateShaderModule(ShaderModuleDescriptor const& desc)
{
    ShaderModuleHandle ret_val= shader_module_pool.Allocate();
    VulkanShaderModule& impl = *shader_module_pool.GetHot(ret_val);

    VkShaderModuleCreateInfo vk_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc.byte_code.Size(),
        .pCode = (const uint32_t*)desc.byte_code.begin(),
    };

    VkResult res = vkCreateShaderModule(vulkan_backend->vk_device, &vk_module_info, nullptr, &impl.vk_shader_module);
    assert(res == VK_SUCCESS);

    return ret_val;
}

void phx::rhi::VulkanResourceManager::DeleteShaderModule(ShaderModuleHandle handle)
{
    VulkanShaderModule* impl = shader_module_pool.GetHot(handle);
    vkDestroyShaderModule(vulkan_backend->vk_device, impl->vk_shader_module, nullptr);
    shader_module_pool.Free(handle);
}

PipelineStateHandle phx::rhi::VulkanResourceManager::CreatePipeline(const PipelineStateDescriptor& desc)
{
    PHX_PROFILE_SECTION("Vulkan::CreatePipeline");

    Handle<PipelineState> ret_val = pipeline_state_pool.Allocate(); // Renamed to snake_case
    VulkanPipelineState& impl = *pipeline_state_pool.GetHot(ret_val); // Corrected access to buffer_pool

    {
        // -- Create Pipeline layout ---
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;       // No descriptor sets
        pipelineLayoutInfo.pSetLayouts = nullptr;    // No layouts
        pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        vkCreatePipelineLayout(vulkan_backend->vk_device, &pipelineLayoutInfo, nullptr, &impl.vk_pipeline_layout);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    //pipelineInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = impl.vk_pipeline_layout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    Span<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,

        // 2. Other Standard Dynamic States you likely want
        VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
        // VK_DYNAMIC_STATE_CULL_MODE,  (If you enabled extendedDynamicState)
        // VK_DYNAMIC_STATE_FRONT_FACE, (If you enabled extendedDynamicState)
        // VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE, (If you enabled extendedDynamicState)
        // VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE (If you enabled extendedDynamicState)
    };

    VkPipelineDynamicStateCreateInfo dynamic_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (uint32_t)dynamic_states.size(),
        .pDynamicStates = dynamic_states.data()
    };

    pipelineInfo.pDynamicState = &dynamic_info;

    // -- Shader Stages ---
    StaticArray<VkPipelineShaderStageCreateInfo, static_cast<size_t>(ShaderStage::Count)> shader_stages;
    size_t num_stages = 0;

    for (auto& stage_info : desc.shader_stages)
    {
        if (!stage_info.module_handle.IsValid())
            continue;

        VulkanShaderModule& impl = *shader_module_pool.GetHot(stage_info.module_handle);

        VkPipelineShaderStageCreateInfo& create_info = shader_stages[num_stages++];
        create_info.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        create_info.stage   = ShaderStageToVulkanShaderStage(stage_info.stage);
        create_info.module  = impl.vk_shader_module; 
        create_info.pName   = stage_info.entry_point; 
    }

    pipelineInfo.stageCount = num_stages;
    pipelineInfo.pStages = shader_stages.begin();

    // -- Primitive type ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch (desc.PrimType)
    {
    case PrimitiveType::PointList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case PrimitiveType::LineList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case PrimitiveType::LineStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        break;
    case PrimitiveType::TriangleStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        break;
    case PrimitiveType::TriangleList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case PrimitiveType::PatchList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        break;
    default:
        break;
    }
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    pipelineInfo.pInputAssemblyState = &inputAssembly;

    // -- Rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_TRUE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    const void** tail = &rasterizer.pNext;

    // depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
    VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateInfo = {};
    depthClipStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    depthClipStateInfo.depthClipEnable = VK_TRUE;

    VkPhysicalDeviceDepthClipEnableFeaturesEXT depthClipEnableFeature = {};
    depthClipEnableFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    *tail = &depthClipStateInfo;
    tail = &depthClipStateInfo.pNext;

    {
        const RasterRenderState& rs = desc.RasterState;

        switch (rs.FillMode)
        {
        case RasterFillMode::Wireframe:
            rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
            break;
        case RasterFillMode::Solid:
        default:
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            break;
        }

        switch (rs.CullMode)
        {
        case RasterCullMode::Back:
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        case RasterCullMode::Front:
            rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case RasterCullMode::None:
        default:
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            break;
        }

        rasterizer.frontFace = rs.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = rs.DepthBias != 0 || rs.SlopeScaledDepthBias != 0;
        rasterizer.depthBiasConstantFactor = static_cast<float>(rs.DepthBias);
        rasterizer.depthBiasClamp = rs.DepthBiasClamp;
        rasterizer.depthBiasSlopeFactor = rs.SlopeScaledDepthBias;

        // Depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
        depthClipStateInfo.depthClipEnable = rs.DepthClipEnable ? VK_TRUE : VK_FALSE;

        VkPipelineRasterizationConservativeStateCreateInfoEXT rasterizationConservativeState = {};
        if (/*CheckCapability(GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION) && */rs.ConservativeRasterEnable)
        {
            rasterizationConservativeState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
            rasterizationConservativeState.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
            rasterizationConservativeState.extraPrimitiveOverestimationSize = 0.0f;
            *tail = &rasterizationConservativeState;
            tail = &rasterizationConservativeState.pNext;
        }
    }
    pipelineInfo.pRasterizationState = &rasterizer;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 0;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 0;
    viewportState.pScissors = nullptr;

    pipelineInfo.pViewportState = &viewportState;

    // -- Depth-Stencil ---
    VkPipelineDepthStencilStateCreateInfo depthstencil = {};
    depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    {
        const DepthStencilRenderState& dss = desc.DepthStencilState;
        depthstencil.depthTestEnable = dss.DepthEnable ? VK_TRUE : VK_FALSE;
        depthstencil.depthWriteEnable = dss.DepthWriteMask == DepthWriteMask::Zero ? VK_FALSE : VK_TRUE;
        depthstencil.depthCompareOp = ConvertComparisonFunc(dss.DepthFunc);

        if (dss.StencilEnable)
        {
            depthstencil.stencilTestEnable = VK_TRUE;

            depthstencil.front.compareMask = dss.StencilReadMask;
            depthstencil.front.writeMask = dss.StencilWriteMask;
            depthstencil.front.reference = 0; // runtime supplied
            depthstencil.front.compareOp = ConvertComparisonFunc(dss.FrontFace.StencilFunc);
            depthstencil.front.passOp = ConvertStencilOp(dss.FrontFace.StencilPassOp);
            depthstencil.front.failOp = ConvertStencilOp(dss.FrontFace.StencilFailOp);
            depthstencil.front.depthFailOp = ConvertStencilOp(dss.FrontFace.StencilDepthFailOp);

            depthstencil.back.compareMask = dss.StencilReadMask;
            depthstencil.back.writeMask = dss.StencilWriteMask;
            depthstencil.back.reference = 0; // runtime supplied
            depthstencil.back.compareOp = ConvertComparisonFunc(dss.BackFace.StencilFunc);
            depthstencil.back.passOp = ConvertStencilOp(dss.BackFace.StencilPassOp);
            depthstencil.back.failOp = ConvertStencilOp(dss.BackFace.StencilFailOp);
            depthstencil.back.depthFailOp = ConvertStencilOp(dss.BackFace.StencilDepthFailOp);
        }
        else
        {
            depthstencil.stencilTestEnable = VK_FALSE;

            depthstencil.front.compareMask = 0;
            depthstencil.front.writeMask = 0;
            depthstencil.front.reference = 0;
            depthstencil.front.compareOp = VK_COMPARE_OP_NEVER;
            depthstencil.front.passOp = VK_STENCIL_OP_KEEP;
            depthstencil.front.failOp = VK_STENCIL_OP_KEEP;
            depthstencil.front.depthFailOp = VK_STENCIL_OP_KEEP;

            depthstencil.back.compareMask = 0;
            depthstencil.back.writeMask = 0;
            depthstencil.back.reference = 0; // runtime supplied
            depthstencil.back.compareOp = VK_COMPARE_OP_NEVER;
            depthstencil.back.passOp = VK_STENCIL_OP_KEEP;
            depthstencil.back.failOp = VK_STENCIL_OP_KEEP;
            depthstencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
        }

#if false
        if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
        {
            depthstencil.depthBoundsTestEnable = dss.DepthBoundsTestEnable ? VK_TRUE : VK_FALSE;
        }
        else
        {
            depthstencil.depthBoundsTestEnable = VK_FALSE;
        }
#else

        depthstencil.depthBoundsTestEnable = VK_FALSE;
#endif
    }

    pipelineInfo.pDepthStencilState = &depthstencil;

    // -- Tessellation ---
#if false
    VkPipelineTessellationStateCreateInfo tessellationInfo = {};
    tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationInfo.patchControlPoints = desc->patch_control_points;

    pipelineInfo.pTessellationState = &tessellationInfo;
#endif

    // -- Input Layout ---
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    PHX_ASSERT(desc.VertexBufferBindings.IsEmpty());
    if (!desc.VertexBufferBindings.IsEmpty())
    {
#if false
        InputLayout* inputLayout = desc.InputLayout;
        uint32_t lastBinding = 0xFFFFFFFF;
        for (auto& e : inputLayout->elements)
        {
            if (e.InputSlot == lastBinding)
                continue;
            lastBinding = e.InputSlot;
            VkVertexInputBindingDescription& bind = bindings.emplace_back();
            bind.binding = e.InputSlot;
            bind.inputRate = e.InputSlotClass == InputClassification::PerVertexData ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
            bind.stride = GetFormatStride(e.Format);
        }

        uint32_t offset = 0;
        uint32_t i = 0;
        lastBinding = 0xFFFFFFFF;
        for (auto& e : inputLayout->elements)
        {
            VkVertexInputAttributeDescription attr = {};
            attr.binding = e.InputSlot;
            if (attr.binding != lastBinding)
            {
                lastBinding = attr.binding;
                offset = 0;
            }
            attr.format = FormatToVkFormat(e.Format);
            attr.location = i;
            attr.offset = e.AlignedByteOffset;
            if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
            {
                // need to manually resolve this from the format spec.
                attr.offset = offset;
                offset += GetFormatStride(e.Format);
            }

            attributes.push_back(attr);

            i++;
        }

        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertexInputInfo.pVertexBindingDescriptions = bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
#endif
    }

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional
    pipelineInfo.pMultisampleState = &multisampling;

    pipelineInfo.renderPass = VK_NULL_HANDLE;

    vulkan_check(
        vkCreateGraphicsPipelines(vulkan_backend->vk_device, vk_pipeline_cache, 1, &pipelineInfo, nullptr, &impl.vk_pipeline));

    return ret_val;
}

void phx::rhi::VulkanResourceManager::DeletePipeline(PipelineStateHandle handle)
{
    deferred_delete_queue.EnqueueDelete({
        frame_number,
        [=, this]() 
        { 
            VulkanPipelineState* impl = pipeline_state_pool.GetHot(handle);
            // TODO: Move into the deconstructor of struct
            if (impl)
            {
                vkDestroyPipeline(vulkan_backend->vk_device, impl->vk_pipeline, nullptr);

                vkDestroyPipelineLayout(vulkan_backend->vk_device, impl->vk_pipeline_layout, nullptr);

                pipeline_state_pool.Free(handle);
            }
        }
    });
}

int VulkanResourceManager::CreateSubResource(VulkanBuffer& buffer, BufferDescriptor const& desc, SubresouceType subresource_type, size_t offset, size_t size)
{
    assert(subresource_type == SubresouceType::SRV || subresource_type == SubresouceType::UAV);

    Format format = desc.Format;

    // Is raw buffer
    if (format == Format::UNKNOWN)
    {
        buffer.srv_is_typed = false;
#if false
        // These sections are commented out in the original code, keeping them commented.
        // If uncommented, they would need  prefix for m_bindlessStorageBuffers and m_device
        // buffer.srv_index = bindless_storage_buffers.Allocate(); // Assuming a new name for this member

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
        // write.dstSet = bindless_storage_buffers.DescritporSetVk; // Assuming new name
        // write.pBufferInfo = &buffer_info;

        // vkUpdateDescriptorSets(vk_device, 1, &write, 0, nullptr);
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

        VkResult res = vkCreateBufferView(vulkan_backend->vk_device, &srv_desc, nullptr, &buffer.buffer_view);
        assert(res == VK_SUCCESS);

        if (subresource_type == SubresouceType::SRV)
        {
            // buffer.srv_index = bindless_uniform_texel_buffers.Allocate(); // Assuming new name
            if (buffer.buffer_view != VK_NULL_HANDLE)
            {
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                write.dstBinding = 0;
                write.dstArrayElement = buffer.srv_index;
                write.descriptorCount = 1;
                // write.dstSet = bindless_uniform_texel_buffers.DescritporSetVk; // Assuming new name
                write.pTexelBufferView = &buffer.buffer_view;
                vkUpdateDescriptorSets(vulkan_backend->vk_device, 1, &write, 0, nullptr);
            }

            return -1;
        }
        else
        {
            // buffer.uav_index = bindless_storage_texel_buffers.Allocate(); // Assuming new name
            if (buffer.buffer_view != VK_NULL_HANDLE)
            {
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                write.dstBinding = 0;
                write.dstArrayElement = buffer.uav_index;
                write.descriptorCount = 1;
                // write.dstSet = bindless_storage_texel_buffers.DescritporSetVk; // Assuming new name
                write.pTexelBufferView = &buffer.buffer_view;
                vkUpdateDescriptorSets(vulkan_backend->vk_device, 1, &write, 0, nullptr);
            }
            return -1;
        }
    }

    return 0;
}

