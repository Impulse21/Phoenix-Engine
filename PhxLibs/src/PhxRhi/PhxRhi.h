#pragma once

#include <PhxRhi/RHICommon.h>

namespace phx::rhi
{
    struct RhiDescriptor
    {
        SwapChainDescriptor SwapChainDesc = {};
        void* WindowsHandle = nullptr;
        uint32_t MaxNumTextures = 1000;
        uint32_t MaxNumGpuBuffers = 1000;
        uint32_t MaxNumPipelineStates = 1000;
    };

    bool Initialize(RhiDescriptor const& desc);
    void Shutdown();
    void SubmitAndPresentFrame();
    void WaitForIdle();

    GpuBufferHandle CreateBuffer(const GpuBufferDescriptor& desc, const void* initialData = nullptr);
    TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr);
    PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc);

    void DeletePipeline(PipelineStateHandle handle);
    void DeleteTexture(TextureHandle handle);
    void DeleteBuffer(GpuBufferHandle handle);

    DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV);
    Budget GetBudget();
    ShaderFormat GetShaderFormat();

    struct CommandRecorder;
    CommandRecorder BeginFrameCommandBuffer(CommandQueueType type = CommandQueueType::Graphics);
    CommandRecorder BeginAsyncCommandBuffer(CommandQueueType type);

    FenceHandle SubmitAsyncCommandBuffer(phx::Span<CommandBufferHandle> contexts);

    namespace command_recorder::impl
    {
        void BindPipelineState(CommandBufferHandle handle, PipelineStateHandle pso);
        void Draw(CommandBufferHandle handle, uint32_t vertex_count, uint32_t start_vertex_location);
        void DrawIndexed(CommandBufferHandle handle, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location);
        void DrawInstanced(CommandBufferHandle handle, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location);
        void DrawIndexedInstanced(CommandBufferHandle handle, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation);
    }

    struct CommandRecorder
    {
    public:
        PHX_FORCE_INLINE void BindPipelineState(PipelineStateHandle pso)
        {
            // Forwards to the *actual* implementation
            command_recorder::impl::BindPipelineState(m_handle, pso);
        }

        PHX_FORCE_INLINE void Draw(uint32_t vertex_count, uint32_t start_vertex_location)
        {
            command_recorder::impl::Draw(m_handle, vertex_count, start_vertex_location);
        }

        PHX_FORCE_INLINE void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
        {
            command_recorder::impl::DrawIndexed(m_handle, index_count, start_index_location, base_vertex_location);
        }

        PHX_FORCE_INLINE void DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
        {
            command_recorder::impl::DrawInstanced(m_handle, vertex_count, instance_count, start_vertex_location, start_instance_location);
        }

        PHX_FORCE_INLINE void DrawIndexedInstanced(uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation)
        {
            command_recorder::impl::DrawIndexedInstanced(m_handle, index_count, instance_count, start_index_location, base_vertex_location, startInstanceLocation);
        }

        // --- Handle Access ---
        // Public getter to retrieve the handle for functions like Submit
        CommandBufferHandle GetHandle() const { return m_handle; }

    public:
        explicit CommandRecorder(CommandBufferHandle handle) : m_handle(handle) {}
        ~CommandRecorder() = default;

        CommandRecorder(const CommandRecorder&) = delete;
        CommandRecorder& operator=(const CommandRecorder&) = delete;

        CommandRecorder(CommandRecorder&& other) noexcept : m_handle(other.m_handle)
        {
            // Invalidate the moved-from object
            other.m_handle = {}; // Or your "null" handle equivalent
        }

        CommandRecorder& operator=(CommandRecorder&& other) noexcept
        {
            if (this != &other)
            {
                // Note: You might need to release/destroy the *current*
                // m_handle if it owns a resource, but here it looks
                // like a non-owning handle.
                m_handle = other.m_handle;
                other.m_handle = {};
            }
            return *this;
        }

    private:
        CommandBufferHandle m_handle; // The only state it holds
    };

}