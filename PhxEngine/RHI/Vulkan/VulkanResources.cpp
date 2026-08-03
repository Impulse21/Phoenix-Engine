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
    return {};
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