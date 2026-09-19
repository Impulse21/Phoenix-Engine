#pragma once


#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>

#include <PhxEngine/Renderer/RendererBase.h>

namespace samples
{
    class CubeRenderer final : public phx::renderer::RendererBase
    {
    public:
        CubeRenderer() = default;

        // TODO: Does this need ot be part of the interface? It's only called by APP
        bool Initialize() override;
        void Shutdown() override;

        // TODO: Maybe hide this within the renderer.
        rhi::DescriptorAllocator& GetDescriptorAllocator() override { return m_tex_descriptor_alloc; }
        rhi::TextureAllocator& GetTextureAllocator() override { return m_texture_allocator; }
        
        rhi::GpuBumpAllocator& GetBufferAllocator();

    private:
        phx::rhi::ShaderModuleHandle m_vertex_shader;
        phx::rhi::ShaderModuleHandle m_fragment_shader;
        phx::rhi::PipelineStateHandle m_cube_pipeline;

        phx::rhi::GpuHeap m_buffer_heap;
        phx::rhi::TextureHeap m_texture_heap;
        phx::rhi::TextureAllocator m_texture_allocator;
        
        phx::rhi::GpuHeap m_texture_descriptor_heap;
        phx::rhi::GpuHeap m_sampler_descriptor_heap;

        phx::rhi::DescriptorAllocator m_tex_descriptor_alloc;

        phx::rhi::GpuBumpAllocator m_buffer_allocator;

    }
}