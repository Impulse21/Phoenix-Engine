#include "PhxRhi/PhxRhi_pch.h"
#include <PhxRhi/PhxRhi.h>

#include <PhxRhi/PhxRhi_Thread.h>

#include "VulkanInternal.h"


using namespace phx;
using namespace phx::rhi;

CmdHandle phx::rhi::BeginCommandBuffer(CommandQueueType queue_type)
{
    // Resolve thread
    const uint32_t thread_id = g_rhi_thread_index;
    vulkan::PerThreadData& thread_data = g_vulkan.submission.per_thread_data[thread_id];

    vulkan::CommandPool& pool = thread_data.command_pools[queue_type];
    VkCommandBuffer vk_cmd_buffer = pool.GetFreeBuffer(thread_id);

    VkCommandBufferBeginInfo being_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(vk_cmd_buffer, &being_info);

    uint32_t index = static_cast<uint32_t>(thread_data.active_command_buffers.size());
    thread_data.active_command_buffers.push_back(vk_cmd_buffer);

    return EncodeCmdHandle(thread_id, index);
}



void phx::rhi::BindPipelineState(rhi::CmdHandle /*handle*/, PipelineStateHandle /*pso*/)
{
}

void phx::rhi::Draw(rhi::CmdHandle handle, uint32_t vertex_count, uint32_t start_vertex_location)
{
    vkCmdDraw(
        ResolveCmd(handle),
        vertex_count,
        1,
        start_vertex_location,
        0);
}

void phx::rhi::DrawIndexed(rhi::CmdHandle handle, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    vkCmdDrawIndexed(
        ResolveCmd(handle),
        index_count,
        1,
        start_index_location,
        base_vertex_location,
        0);
}

void phx::rhi::DrawInstanced(rhi::CmdHandle handle, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
{
    vkCmdDraw(
        ResolveCmd(handle),
        vertex_count,
        instance_count,
        start_vertex_location,
        start_instance_location);
}

void phx::rhi::DrawIndexedInstanced(rhi::CmdHandle handle, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
    vkCmdDrawIndexed(
        ResolveCmd(handle),
        index_count,
        instance_count,
        start_index_location,
        base_vertex_location,
        start_instance_location);
}

void phx::rhi::BeginRendering(rhi::CmdHandle handle, SwapchainHandle swapchain_handle, rhi::ClearValue const& clear_value)
{
    InsertSwapchainBarrier(handle, swapchain_handle, ResourceStates::RenderTarget);

    VulkanSwapchainFrame* swapchain = g_vulkan.swapchain_pool.GetHot(swapchain_handle);
    VkClearValue vk_clear_value = {
        .color = {
            .float32 = { clear_value.Colour.R, clear_value.Colour.G, clear_value.Colour.B, clear_value.Colour.A }
        }
    };

    VkRenderingAttachmentInfo color_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->vk_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = vk_clear_value // Use this clear value
    };

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {.x = 0u, .y = 0u},
            .extent = swapchain->vk_swapchain_extent,
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(ResolveCmd(handle), &rendering_info);
}

void phx::rhi::EndRendering(rhi::CmdHandle handle)
{
    vkCmdEndRendering(ResolveCmd(handle));
}

void phx::rhi::InsertSwapchainBarrier(rhi::CmdHandle handle, SwapchainHandle swapchain_handle, rhi::ResourceStates resource_state)
{
    VulkanSwapchainFrame* swapchain_impl = g_vulkan.swapchain_pool.GetHot(swapchain_handle);

    VkImageLayout old_layout = ResourceStateToImageLayout(swapchain_impl->resource_state);
    VkImageLayout new_layout = ResourceStateToImageLayout(resource_state);

    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(swapchain_impl->resource_state);
    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(resource_state);

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = src_stage,
        .srcAccessMask = ResourceStateToAccessFlags2(swapchain_impl->resource_state),
        .dstStageMask = dest_stage,
        .dstAccessMask = ResourceStateToAccessFlags2(resource_state),
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_impl->vk_image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    vkCmdPipelineBarrier2(ResolveCmd(handle), &dependency_info);

    swapchain_impl->resource_state = resource_state;
}

void phx::rhi::InsertBarriers(rhi::CmdHandle handle, Span<GpuBarrier> barriers)
{
    if (barriers.IsEmpty())
        return;

    constexpr size_t MAX_BARRIER_COUNT = 16;
    std::array<VkMemoryBarrier2, MAX_BARRIER_COUNT> vk_mem_barriers;
    std::array<VkBufferMemoryBarrier2, MAX_BARRIER_COUNT> vk_buffer_barriers;
    std::array<VkImageMemoryBarrier2, MAX_BARRIER_COUNT> vk_texture_barriers;

    uint32_t mem_barrier_count = 0;
    uint32_t buffer_barrier_count = 0;
    uint32_t texture_barrier_count = 0;

    VkPipelineStageFlags all_src_stage_mask = 0;
    VkPipelineStageFlags all_dst_stage_mask = 0;
    for (auto& barrier : barriers)
    {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, rhi::GpuBarrier::GlobalBarrier>)
                {
                    if (mem_barrier_count == MAX_BARRIER_COUNT)
                        return;

                    // --- Global Barrier ---
                    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(arg.before_state);
                    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(arg.after_state);
                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    vk_mem_barriers[mem_barrier_count++] = {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = src_stage,
                        .srcAccessMask = ResourceStateToAccessFlags2(arg.before_state),
                        .dstStageMask = dest_stage,
                        .dstAccessMask = ResourceStateToAccessFlags2(arg.after_state)
                    };
                }
                else if constexpr (std::is_same_v<T, GpuBarrier::BufferBarrier>)
                {
                    if (buffer_barrier_count== MAX_BARRIER_COUNT)
                        return;

                    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(arg.before_state);
                    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(arg.after_state);

                    uint64_t mask = 0;
                    const VkPhysicalDeviceFeatures& device_features = g_vulkan.vk_physical_device_features;
                    if (!device_features.tessellationShader)
                        mask |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;

                    if (!device_features.geometryShader)
                        mask |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

                    if (!EnumHasAnyFlags(g_vulkan.capabilities, DeviceCapability::RayTracing))
                        mask |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

                    dest_stage &= ~mask;

                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    VulkanBuffer* vulkan_buffer = g_vulkan.buffer_pool.GetHot(arg.buffer);
                    VkBufferMemoryBarrier2& buffer_barrier = vk_buffer_barriers[buffer_barrier_count++];
                    buffer_barrier = {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = src_stage,
                        .srcAccessMask = ResourceStateToAccessFlags2(arg.before_state),
                        .dstStageMask = dest_stage,
                        .dstAccessMask = ResourceStateToAccessFlags2(arg.after_state),
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .buffer = vulkan_buffer->vk_buffer,
                        .offset = arg.offset,
                        .size = (arg.size == ~0u) ? VK_WHOLE_SIZE : arg.size
                    };
                }
                else if constexpr (std::is_same_v<T, GpuBarrier::TextureBarrier>)
                {
#if false
                    if (texture_barrier_count == MAX_BARRIER_COUNT)
                        return;

                    // --- Texture Barrier ---
                    VkPipelineStageFlags src_stage = ConvertPipelineStages(arg.before_state);
                    VkPipelineStageFlags dest_stage = ConvertPipelineStages(arg.after_state);
                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    vk_texture_barriers[texture_barrier_count++] = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = src_stage,
                        .srcAccessMask = ResourceStateToAccessFlags2(arg.before_state),
                        .dstStageMask = dest_stage,
                        .dstAccessMask = ResourceStateToAccessFlags2(arg.after_state),
                        .oldLayout = ConvertImageLayout(arg.before_state),
                        .newLayout = ConvertImageLayout(arg.after_state),
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = arg.texture,
                        .subresourceRange = {
                            .aspectMask = TranslateImageAspects(arg.Aspects),
                            .baseMipLevel = static_cast<uint32_t>(arg.FirstMip),
                            .levelCount = (arg.mip == -1) ? VK_REMAINING_MIP_LEVELS : static_cast<uint32_t>(arg.mip),
                            .baseArrayLayer = static_cast<uint32_t>(arg.FirstSlice),
                            .layerCount = (arg.slice == -1) ? VK_REMAINING_ARRAY_LAYERS : static_cast<uint32_t>(arg.slice)
                        }
                     });
#endif
                }
            },
            barrier.Data
        );
    }

    if (all_src_stage_mask == 0) 
        all_src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (all_dst_stage_mask == 0) 
        all_dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    // --- Select the correct data pointer (stack or heap) ---
    const VkMemoryBarrier2* memory_barriers_ptr = vk_mem_barriers.data();
    const VkBufferMemoryBarrier2* buffer_barriers_ptr = vk_buffer_barriers.data();
    const VkImageMemoryBarrier2* image_barriers_ptr = vk_texture_barriers.data();

    VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0, // or VK_DEPENDENCY_BY_REGION_BIT
        .memoryBarrierCount = static_cast<uint32_t>(mem_barrier_count),
        .pMemoryBarriers = memory_barriers_ptr,
        .bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barrier_count),
        .pBufferMemoryBarriers = buffer_barriers_ptr,
        .imageMemoryBarrierCount = static_cast<uint32_t>(texture_barrier_count),
        .pImageMemoryBarriers = image_barriers_ptr
    };

    vkCmdPipelineBarrier2(ResolveCmd(handle), &dependency_info);
}

void phx::rhi::CopyBuffer(
    rhi::CmdHandle handle,
    BufferHandle src_buffer,
    uint64_t src_offset,
    BufferHandle dest_buffer,
    uint64_t dest_offset,
    size_t size)
{
    VkBufferCopy2 bufferCopyRegion = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .pNext = nullptr,
        .srcOffset = src_offset,
        .dstOffset = dest_offset,
        .size = size,
    };
    

    VulkanBuffer* src_buffer_impl = g_vulkan.buffer_pool.GetHot(src_buffer);
    VulkanBuffer* dest_buffer_impl = g_vulkan.buffer_pool.GetHot(dest_buffer);

    // Define the overall copy operation
    VkCopyBufferInfo2 copyBufferInfo = { 
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .pNext = nullptr,
        .srcBuffer = src_buffer_impl->vk_buffer,
        .dstBuffer = dest_buffer_impl->vk_buffer,
        .regionCount = 1,
        .pRegions = &bufferCopyRegion,
    };
    

    // Record the command into the command buffer
    vkCmdCopyBuffer2(ResolveCmd(handle), &copyBufferInfo);
}



struct VulkanSubmissionManager : public ISubmissionManager
{

    VulkanBackend* vulkan_backend;
    VulkanResourceManager* vulkan_resource_manager;
    std::unique_ptr<PerThreadData[]> per_thread_data;

    struct PerQueueSync
    {
        VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
        std::atomic_uint64_t fence_counter = 1;
    };
    EnumArray<PerQueueSync, CommandQueueType> per_queue_syncs;

    StaticArray<FenceHandle, cMaxInflightFrames> frame_fences = { .data = {{}, {}} };

    size_t frame_number = 0;
    size_t num_threads = 0;

    struct InflightCommandBuffer
    {
        VulkanCommandBuffer* buffer;
        FenceHandle				fence_handle;
    };
    std::vector<InflightCommandBuffer> inflight_cmd_queue;
    std::mutex inflight_commands_queue_mutex;

    struct InflightUpload
    {
        uint64_t fence_value = 0;;
        uint32_t thread_id;
        uint64_t head_offset;
    };
    std::vector<InflightUpload> inflight_upload_queue;

    struct PendingDeletion
    {
        uint64_t fence_value = 0;
        BufferHandle buffer = {};
    };
    std::vector<PendingDeletion> pending_one_off_deletions;
    std::mutex upload_tracking_mutex;

    // -- Interface implementation ---
    bool Initialize() override;
    void Shutdown() override;

    void BeginFrame(SwapchainHandle swapChain) override;
    void EndFrame(
        SwapchainHandle swapChain,
        Span<ICommandBuffer*> graphics_buffers,
        Span<FenceHandle> wait_fences = {}) override;

    void WaitForIdle() override;
    bool IsFenceCompleted(FenceHandle handle) override;

    StagingBlock RequestStagingMemory(uint32_t size, uint32_t aligmnet = 16) override;

    ICommandBuffer* BeginCommandBuffer(CommandQueueType queue_type) override;
    FenceHandle Submit(
        CommandQueueType queue_type,
        Span<ICommandBuffer*> cmd_buffers,
        Span<FenceHandle> wait_fences = {}) override;

    VulkanSubmissionManager(VulkanBackend* vulkan_backend, VulkanResourceManager* vulkan_resource_manager, size_t thread_count);
    ~VulkanSubmissionManager() override = default;

private:
    friend PerThreadData;

    size_t GetCurrentFrameIndex() { return frame_number % cMaxInflightFrames; }
    void RetireCommandBuffers(Span<ICommandBuffer*> command_buffers, FenceHandle fence_value);
    void ReclaimFinishedCommandBuffers();
    void ReclaimFinishedUploads();

    FenceHandle SubmitInternal(
        CommandQueueType queue_type,
        Span<ICommandBuffer*> cmd_buffers,
        Span<FenceHandle> wait_fences,
        Span<VkSemaphore> binary_wait_sems,
        Span<VkSemaphore> binary_signal_sems,
        VkPipelineStageFlags flags);
};

}
struct StagingRingBuffer
{
    BufferHandle buffer_handle = {};
    std::byte* mapped_ptr = nullptr;
    uint64_t size = 0ull;
    uint64_t mask = 0ull;
    uint64_t head = 0ull;

    // Main thread writes tot his
    std::atomic_uint64_t tail;

    Result<StagingBlock> Allocate(uint64_t alloc_size, uint32_t alignment);
    void Initialize(VulkanSubmissionManager* sub_manager);
    void Shutdown(VulkanSubmissionManager* sub_manager);
};

struct PerThreadData
{
    struct CommandPool
    {
        CommandQueueType queue_type;
        VulkanResourceManager* vulkan_resource_manager;
        VulkanBackend* vulkan_backend;
        VkCommandPool vk_cmd_pool;
        std::vector<std::unique_ptr<phx::rhi::VulkanCommandBuffer>> buffer_pool;
        std::vector<phx::rhi::VulkanCommandBuffer*> free_buffers;

        phx::rhi::VulkanCommandBuffer* GetFreeBuffer(uint32_t thread_id);
    };

    uint32_t thread_id = 0;
    VulkanSubmissionManager* sub_manager;

    EnumArray<CommandPool, CommandQueueType> command_pools;

    // -- Upload manager info ---
    // TODO: Move to it's own class.

    std::vector<BufferHandle> active_one_off_buffers;
    StagingRingBuffer staging_ring_buffer;

    StagingBlock RequestStagingBlock(size_t size, uint32_t alignment);
    StagingBlock CreateOneShotUploadBuffer(size_t size, uint32_t alignment);

    void Initialize(VulkanSubmissionManager* sub_manager, uint32_t thread_id);
    void Shutdown();
};


bool VulkanSubmissionManager::Initialize()
{
    PHX_PROFILE;
    if (vulkan_backend->vk_features_1_2.timelineSemaphore != VK_TRUE)
    {
        PHX_RHI_ERROR("Required VK 1.2 feature - Timeline Semaphore is not available on this device.");
        return false;
    }
    VkSemaphoreTypeCreateInfo timeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = NULL,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0
    };

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_create_info,
        .flags = 0,
    };
    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        VkResult result = vkCreateSemaphore(vulkan_backend->vk_device, &semaphore_create_info, NULL, &per_queue_syncs[q].vk_timeline_semaphore);
        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create queue timeline semaphore");
    }

    PHX_RHI_INFO("Initializing Per thread Command data.");
    per_thread_data = std::make_unique<PerThreadData[]>(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
    {
        per_thread_data[i].Initialize(this, i);
    }

    return true;
}

void phx::rhi::VulkanSubmissionManager::Shutdown()
{
    WaitForIdle();
    vulkan_resource_manager->RunGarbageCollection(~0u);

    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        vkDestroySemaphore(vulkan_backend->vk_device, per_queue_syncs[q].vk_timeline_semaphore, nullptr);
    }
    for (size_t i = 0; i < num_threads; ++i)
    {
        per_thread_data[i].Shutdown();
    }
}

void phx::rhi::VulkanSubmissionManager::BeginFrame(SwapchainHandle swapchain)
{
    const size_t frame_index = GetCurrentFrameIndex();
    const FenceHandle frame_to_wait_for = frame_fences[frame_index];

    if (frame_to_wait_for.value > 0)
    {
        VkSemaphore wait_timline_sem = per_queue_syncs[frame_to_wait_for.queue_type].vk_timeline_semaphore;
        VkSemaphoreWaitInfo wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &wait_timline_sem,
            .pValues = &frame_to_wait_for.value
        };

        vkWaitSemaphores(
            vulkan_backend->vk_device,
            &wait_info,
            UINT64_MAX
        );
    }

    ReclaimFinishedCommandBuffers();
    ReclaimFinishedUploads();

    VulkanSwapchain* swapchain_impl = vulkan_resource_manager->swapchain_pool.GetCold(swapchain);
    if (!swapchain_impl)
    {
        PHX_RHI_CRITICAL("Unable to local swapchain when ending frame.");
        return;
    }

    VulkanSwapchainFrame* impl_frame = vulkan_resource_manager->swapchain_pool.GetHot(swapchain);
    impl_frame->vk_image_available_sem = swapchain_impl->vk_image_available_sem[frame_index];

    uint32_t image_index;
    vkAcquireNextImageKHR(
        vulkan_backend->vk_device,
        swapchain_impl->vk_swapchain,
        UINT64_MAX,
        impl_frame->vk_image_available_sem,
        VK_NULL_HANDLE,
        &image_index);

    impl_frame->image_index = static_cast<uint8_t>(image_index);
    impl_frame->vk_render_finished_sem = swapchain_impl->vk_render_finished_sem[image_index];
    impl_frame->vk_image = swapchain_impl->vk_images[image_index];
    impl_frame->vk_image_view = swapchain_impl->vk_image_views[image_index];
    impl_frame->resource_state = ResourceStates::Unknown;
}

void phx::rhi::VulkanSubmissionManager::EndFrame(
    SwapchainHandle swapchain,
    Span<ICommandBuffer*> graphics_buffers,
    Span<FenceHandle> wait_fences)
{
    if (graphics_buffers.IsEmpty())
        return;

    VulkanSwapchainFrame* swapchain_impl_frame = vulkan_resource_manager->swapchain_pool.GetHot(swapchain);
    if (!swapchain_impl_frame)
    {
        PHX_RHI_CRITICAL("Unable to local swapchain when ending frame.");
        return;
    }

    const uint64_t current_fame_index = GetCurrentFrameIndex();
    frame_fences[current_fame_index] =
        SubmitInternal(
            CommandQueueType::Graphics,
            graphics_buffers,
            wait_fences,
            { swapchain_impl_frame->vk_image_available_sem },
            { swapchain_impl_frame->vk_render_finished_sem },
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);


    const uint32_t image_index = static_cast<uint32_t>(swapchain_impl_frame->image_index);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain_impl_frame->vk_render_finished_sem,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_impl_frame->vk_swapchain,
        .pImageIndices = &image_index,
    };

    VulkanBackend::Queue& queue = vulkan_backend->queues[CommandQueueType::Graphics];
    vkQueuePresentKHR(queue.vk_queue, &present_info);

    frame_number++;
}

void VulkanSubmissionManager::WaitForIdle()
{
    PHX_CORE_ASSERT(vulkan_backend->vk_device != VK_NULL_HANDLE);
    vkDeviceWaitIdle(vulkan_backend->vk_device);
}

bool phx::rhi::VulkanSubmissionManager::IsFenceCompleted(FenceHandle fence_handle)
{
    PerQueueSync& queue_sync = per_queue_syncs[fence_handle.queue_type];

    uint64_t completed_value = 0;

    VkResult result = vkGetSemaphoreCounterValue(
        vulkan_backend->vk_device,
        queue_sync.vk_timeline_semaphore,
        &completed_value);

    PHX_CORE_ASSERT(result == VK_SUCCESS, "Failed to retrieve timeline semaphore's completed value")
        if (result != VK_SUCCESS)
        {
            return false;
        }

    return fence_handle.value <= completed_value;
}

StagingBlock phx::rhi::VulkanSubmissionManager::RequestStagingMemory(uint32_t size, uint32_t aligmnet)
{
    const uint32_t thread_id = g_rhi_thread_index;
    PerThreadData& thread_data = per_thread_data[thread_id];

    return thread_data.RequestStagingBlock(size, aligmnet);
}

ICommandBuffer* VulkanSubmissionManager::BeginCommandBuffer(CommandQueueType queue_type)
{
    const uint32_t thread_index = g_rhi_thread_index;

    PerThreadData& thread_data = per_thread_data[thread_index];
    PerThreadData::CommandPool& pool = thread_data.command_pools[queue_type];

    VulkanCommandBuffer* cmd_buffer = pool.GetFreeBuffer(thread_index);
    cmd_buffer->Begin();

    return cmd_buffer;
}

FenceHandle VulkanSubmissionManager::Submit(
    CommandQueueType queue_type,
    Span<ICommandBuffer*> cmd_buffers,
    Span<FenceHandle> wait_fences)
{
    // Using VK_PIPELINE_STAGE_ALL_COMMANDS_BIT as it's a safe choice.
    return SubmitInternal(queue_type, cmd_buffers, wait_fences, {}, {}, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

phx::rhi::VulkanCommandBuffer* PerThreadData::CommandPool::GetFreeBuffer(uint32_t thread_id)
{
    if (!free_buffers.empty())
    {
        phx::rhi::VulkanCommandBuffer* buffer = free_buffers.back();
        free_buffers.pop_back();

        return buffer;
    }

    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    // 2. Actually create the handle
    VkCommandBuffer vk_buffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(vulkan_backend->vk_device, &cmd_alloc_info, &vk_buffer);

    auto& vulkan_cmd_buffer = buffer_pool.emplace_back(
        std::make_unique<VulkanCommandBuffer>(vulkan_resource_manager, vk_buffer, queue_type, thread_id));

    return vulkan_cmd_buffer.get();
}

void phx::rhi::VulkanSubmissionManager::RetireCommandBuffers(Span<ICommandBuffer*> command_buffers, FenceHandle fence_value)
{
    std::scoped_lock _(inflight_commands_queue_mutex);

    for (auto cmd_buffer : command_buffers)
    {
        inflight_cmd_queue.push_back({
                .buffer = static_cast<VulkanCommandBuffer*>(cmd_buffer),
                .fence_handle = fence_value,
            });
    }
}

void phx::rhi::VulkanSubmissionManager::ReclaimFinishedCommandBuffers()
{
    EnumArray<uint64_t, CommandQueueType> completed_values = {};
    VkResult result;
    for (size_t i = 0; i < static_cast<uint32_t>(CommandQueueType::Count); ++i)
    {
        // This is the key function:
        result = vkGetSemaphoreCounterValue(
            vulkan_backend->vk_device,
            per_queue_syncs[i].vk_timeline_semaphore,
            &completed_values[i]);

        if (result != VK_SUCCESS)
        {
            PHX_RHI_ERROR("Failed to get semaphore fence value");
            completed_values[i] = 0; // Or last known good value
        }
    }

    std::scoped_lock _(inflight_commands_queue_mutex);
    std::erase_if(inflight_cmd_queue,
        [&](const InflightCommandBuffer& pending) {
            const FenceHandle& fence = pending.fence_handle;

            if (fence.value <= completed_values[fence.queue_type])
            {
                vkResetCommandBuffer(pending.buffer->vk_handle, 0);

                const uint32_t thread_id = pending.buffer->thread_id;
                PerThreadData& thread_data = per_thread_data[thread_id];
                PerThreadData::CommandPool& pool = thread_data.command_pools[pending.buffer->queue_type];
                pool.free_buffers.push_back(pending.buffer);

                return true;
            }

            return false;
        });

}

void phx::rhi::VulkanSubmissionManager::ReclaimFinishedUploads()
{
    uint64_t completed_fence_value = 0;

    VkResult result = vkGetSemaphoreCounterValue(
        vulkan_backend->vk_device,
        per_queue_syncs[CommandQueueType::Copy].vk_timeline_semaphore,
        &completed_fence_value);

    if (result != VK_SUCCESS)
    {
        PHX_RHI_ERROR("Failed to get semaphore fence value");
        completed_fence_value = 0; // Or last known good value
    }

    static thread_local std::vector<uint64_t> s_new_tail_values(num_threads, 0);

    std::scoped_lock _(upload_tracking_mutex);

    auto it = inflight_upload_queue.begin();
    while (it != inflight_upload_queue.end())
    {
        InflightUpload& upload = *it;

        // Check if this upload's fence is complete
        if (upload.fence_value <= completed_fence_value)
        {
            uint64_t& current_max = s_new_tail_values[upload.thread_id];
            current_max = std::max(current_max, upload.head_offset);

            it = inflight_upload_queue.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (uint32_t thread_id = 0; thread_id < num_threads; ++thread_id)
    {
        if (s_new_tail_values[thread_id] > 0)
        {
            per_thread_data[thread_id].staging_ring_buffer.tail.store(
                s_new_tail_values[thread_id],
                std::memory_order_release);

            // Reset our temp value
            s_new_tail_values[thread_id] = 0;
        }
    }

    std::erase_if(pending_one_off_deletions,
        [&](const PendingDeletion& pending) {

            if (pending.fence_value <= completed_fence_value)
            {
                vulkan_resource_manager->DeleteBufferImmediate(pending.buffer);
                return true;
            }

            return false;
        });
}

StagingBlock PerThreadData::RequestStagingBlock(size_t size, uint32_t alignment)
{
    if (size > UPLOAD_RING_BUFFER_SIZE)
        return CreateOneShotUploadBuffer(size, alignment);

    Result<StagingBlock> result = staging_ring_buffer.Allocate(size, alignment);
    if (result)
    {
        return result.GetValue();
    }

    sub_manager->ReclaimFinishedUploads();

    result = staging_ring_buffer.Allocate(size, alignment);
    if (result)
    {
        return result.GetValue();
    }

    PHX_RHI_WARN("Staging buffer fragmented! Promoting to one-off");
    return CreateOneShotUploadBuffer(size, alignment);
}

StagingBlock PerThreadData::CreateOneShotUploadBuffer(size_t size, uint32_t alignment)
{
    VulkanResourceManager* vulkan_rm = sub_manager->vulkan_resource_manager;

    const size_t aligned_size = AlignUp(size, alignment);
    BufferHandle buffer_handle = g_vulkan.CreateBuffer({
        .DebugName = "One_shot_bufffer",
        .Size = static_cast<uint32_t>(aligned_size),
        .Usage = Usage::Upload,
        .MiscFlags = ResourceMiscFlags::BufferRaw,
        .InitialState = ResourceStates::CopySource
        });

    VulkanBuffer* vulkan_buffer = g_vulkan.buffer_pool.GetHot(buffer_handle);
    StagingBlock block = {
        .data_ptr = vulkan_buffer->mapped_data,
        .size = size,
        .buffer_handle = buffer_handle,
        .gpu_offset = 0
    };

    return block;
}

void PerThreadData::Initialize(VulkanSubmissionManager* sub_manager, uint32_t thread_id)
{
    this->sub_manager = sub_manager;
    this->thread_id = thread_id;

    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        PerThreadData::CommandPool& pool = command_pools[q];
        pool.queue_type = static_cast<CommandQueueType>(q);
        pool.vulkan_resource_manager = sub_manager->vulkan_resource_manager;
        pool.vulkan_backend = sub_manager->vulkan_backend;

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = sub_manager->vulkan_backend->queues[q].vk_queue_family,
        };

        VkResult result = vkCreateCommandPool(sub_manager->vulkan_backend->vk_device, &pool_info, nullptr, &pool.vk_cmd_pool);
        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create command pool");
    }

    staging_ring_buffer.Initialize(sub_manager);
}

void phx::rhi::PerThreadData::Shutdown()
{
    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        vkDestroyCommandPool(sub_manager->vulkan_backend->vk_device, command_pools[q].vk_cmd_pool, nullptr);
    }

    staging_ring_buffer.Shutdown(sub_manager);
}

FenceHandle phx::rhi::VulkanSubmissionManager::SubmitInternal(
    CommandQueueType queue_type,
    Span<ICommandBuffer*> cmd_buffers,
    Span<FenceHandle> wait_fences,
    Span<VkSemaphore> binary_wait_sems,
    Span<VkSemaphore> binary_signal_sems,
    VkPipelineStageFlags flags)
{
    PerQueueSync& queue_sync = per_queue_syncs[queue_type];
    const FenceHandle fence_handle = {
        .value = queue_sync.fence_counter.fetch_add(1),
        .queue_type = queue_type
    };

    static thread_local std::vector<VkCommandBuffer> s_vk_cmd_buffers;
    {
        s_vk_cmd_buffers.clear();
        s_vk_cmd_buffers.reserve(cmd_buffers.size());

        for (auto& cmd_buffer : cmd_buffers)
        {
            auto vulkan_cmd_buffer = static_cast<VulkanCommandBuffer*>(cmd_buffer);
            vulkan_cmd_buffer->End();

            VkCommandBuffer vk_handle = vulkan_cmd_buffer->vk_handle;
            s_vk_cmd_buffers.push_back(vk_handle);
        }
    }

    static thread_local std::vector<uint64_t> s_wait_fence_values;
    {
        s_wait_fence_values.clear();
        s_wait_fence_values.reserve(binary_wait_sems.size() + wait_fences.size());

        for (size_t i = 0; i < binary_wait_sems.size(); ++i)
            s_wait_fence_values.push_back(0);

        for (auto& wait_fence : wait_fences)
            s_wait_fence_values.push_back(wait_fence.value);
    }

    static thread_local std::vector<VkSemaphore> s_wait_semaphores;
    {
        s_wait_semaphores.clear();
        s_wait_semaphores.reserve(binary_wait_sems.size() + wait_fences.size());

        for (auto& binary_semaphore : binary_wait_sems)
            s_wait_semaphores.push_back(binary_semaphore);

        for (size_t i = 0; i < wait_fences.size(); ++i)
            s_wait_semaphores.push_back(queue_sync.vk_timeline_semaphore);
    }

    static thread_local std::vector<VkPipelineStageFlags> s_wait_stages;
    {
        s_wait_stages.clear();
        s_wait_stages.resize(binary_wait_sems.size() + wait_fences.size());

        std::fill(s_wait_stages.begin(), s_wait_stages.end(), flags);
    }

    static thread_local std::vector<uint64_t> s_signal_fence_values;
    {
        s_signal_fence_values.clear();
        s_signal_fence_values.reserve(binary_signal_sems.size() + 1);

        for (size_t i = 0; i < binary_signal_sems.size(); ++i)
            s_signal_fence_values.push_back(0);

        s_signal_fence_values.push_back(fence_handle.value);
    }

    static thread_local std::vector<VkSemaphore> s_signal_semaphores;
    {
        s_signal_semaphores.clear();
        s_signal_semaphores.reserve(binary_signal_sems.size() + 1);

        for (auto& binary_signal_sem : binary_signal_sems)
            s_signal_semaphores.push_back(binary_signal_sem);

        s_signal_semaphores.push_back(queue_sync.vk_timeline_semaphore);
    }

    VkTimelineSemaphoreSubmitInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = static_cast<uint32_t>(s_wait_fence_values.size()),
        .pWaitSemaphoreValues = s_wait_fence_values.data(),
        .signalSemaphoreValueCount = static_cast<uint32_t>(s_signal_fence_values.size()),
        .pSignalSemaphoreValues = s_signal_fence_values.data(),
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timeline_info,

        .waitSemaphoreCount = static_cast<uint32_t>(s_wait_semaphores.size()),
        .pWaitSemaphores = s_wait_semaphores.data(),
        .pWaitDstStageMask = s_wait_stages.data(),

        .commandBufferCount = static_cast<uint32_t>(s_vk_cmd_buffers.size()),
        .pCommandBuffers = s_vk_cmd_buffers.data(),

        .signalSemaphoreCount = static_cast<uint32_t>(s_signal_semaphores.size()),
        .pSignalSemaphores = s_signal_semaphores.data(),
    };

    // Get queue
    VulkanBackend::Queue& queue = vulkan_backend->queues[queue_type];
    vkQueueSubmit(queue.vk_queue, 1, &submit_info, VK_NULL_HANDLE);

    RetireCommandBuffers(cmd_buffers, fence_handle);

    if (queue_type == CommandQueueType::Copy)
    {
        uint32_t thread_index = g_rhi_thread_index;
        PerThreadData& thread_data = per_thread_data[thread_index];
        std::scoped_lock _(upload_tracking_mutex);

        InflightUpload& inflight_data = inflight_upload_queue.emplace_back();
        inflight_data.fence_value = fence_handle.value;
        inflight_data.thread_id = thread_index;
        inflight_data.head_offset = thread_data.staging_ring_buffer.head;

        for (auto& one_off_buffer : thread_data.active_one_off_buffers)
        {
            PendingDeletion& pending = pending_one_off_deletions.emplace_back();
            pending.fence_value = fence_handle.value;
            pending.buffer = one_off_buffer;
        }
        thread_data.active_one_off_buffers.clear();
    }

    return fence_handle;
}

Result<StagingBlock> phx::rhi::StagingRingBuffer::Allocate(uint64_t alloc_size, uint32_t alignment)
{
    uint64_t aligned_head = AlignUp(alloc_size, alignment);
    uint64_t new_head = aligned_head + alloc_size;
    uint64_t current_tail = tail.load(std::memory_order_acquire);
    if ((new_head - current_tail) > size)
        return make_unexpected(1ull);

    head = new_head;

    StagingBlock staging_block = {
        .data_ptr = mapped_ptr + (aligned_head & mask),
        .size = alloc_size,
        .buffer_handle = buffer_handle,
        .gpu_offset = (aligned_head & mask)
    };

    return staging_block;
}

void phx::rhi::StagingRingBuffer::Initialize(VulkanSubmissionManager* sub_manager)
{

    VulkanResourceManager* vulkan_rm = sub_manager->vulkan_resource_manager;

    size = UPLOAD_RING_BUFFER_SIZE;
    mask = size - 1;
    head = 0;

    buffer_handle = g_vulkan.CreateBuffer({
        .DebugName = "One_shot_bufffer",
        .Size = static_cast<uint32_t>(UPLOAD_RING_BUFFER_SIZE),
        .Usage = Usage::Upload,
        .MiscFlags = ResourceMiscFlags::BufferRaw,
        .InitialState = ResourceStates::CopySource
        });

    VulkanBuffer* vulkan_buffer = g_vulkan.buffer_pool.GetHot(buffer_handle);
    mapped_ptr = static_cast<std::byte*>(vulkan_buffer->mapped_data);
}

void phx::rhi::StagingRingBuffer::Shutdown(VulkanSubmissionManager* sub_manager)
{
    VulkanResourceManager* vulkan_rm = sub_manager->vulkan_resource_manager;
    g_vulkan.DeleteBufferImmediate(buffer_handle);
}
