#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Memory/MemoryHelpers.h>
#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>
#include <PhxEngine/RHI/GpuMemory/DescriptorAllocator.h>

#include <hlsl++.h>

#include "Shaders/Cube_interop.h"

namespace samples
{
    struct Mesh
    {
        phx::rhi::GpuCpuRange<Vertex> vertices;
        phx::rhi::GpuCpuRange<u32> indices;
    };
    struct RenderPacket
    {
        hlslpp::float4x4 mvp;
        Mesh* mesh;
    };

    class CubeRenderer final
    {
    public:
        CubeRenderer() = default;

        bool Initialize();
        void Shutdown();

        void CacheCubeRenderPacket(phx::FramePtr<RenderPacket> renderPacket)
        {
            m_cached_render_packet = renderPacket;
        }

        void Render(phx::rhi::CommandBuffer cmd);

        phx::rhi::GpuBumpAllocator& GetBufferAllocator() { return m_buffer_allocator; }

    private:
        phx::FramePtr<RenderPacket> m_cached_render_packet;

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
    };
}