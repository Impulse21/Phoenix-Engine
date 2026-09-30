#pragma once

#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/StaticArray.h>

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Renderer/HdrRenderTargets.h>

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>
#include <PhxEngine/RHI/GpuMemory/DescriptorAllocator.h>

#include <PhxEngine/ECS/World.h>

#include <hlsl++.h>
namespace horde
{
    enum class Pso : u32
    {
        Capsule,
        Plane,
        Box,
        Count,
    };

    constexpr u32 kPsoCount = static_cast<u32>(Pso::Count);

    class HordeRenderer
    {
    public:
        [[nodiscard]] bool Initialize() noexcept;
        void Shutdown();

        void PreRender(const phx::ecs::World& world, phx::FrameAllocator& frame_allocator);
        void Render();

    private:
        phx::rhi::PipelineStateHandle CreatePso(phx::Span<phx::rhi::ShaderStageInfo> shader_tages) const;

        phx::rhi::GpuBumpAllocator& GetFrameGpuAllocaor()
        {
            const usize index = phx::rhi::GetFrameIndex();
            PHX_ASSERT(index < m_frame_allocators.Size());

            return m_frame_allocators[index];
        }

    private:
        struct RenderPacket
        {
            phx::rhi::PipelineStateHandle pso_handle;
            phx::rhi::GpuRange instance_ptr;
            u32 instance_count;
        };

        struct RenderList
        {
            u32 num_render_packets;
            phx::FramePtr<RenderPacket> render_packets;
        };


    private:
        phx::FramePtr<RenderList> m_curr_render_list;

        // TODO: Add Render Targets
        phx::EnumArray<phx::rhi::PipelineStateHandle, Pso> m_pso;

        phx::rhi::TextureHeap m_texture_heap;
        phx::rhi::GpuHeap m_texture_descriptor_heap;
        phx::rhi::GpuHeap m_sampler_descriptor_heap;
        phx::rhi::GpuHeap m_rebar_heap;

        // host and device visible heap
        phx::StaticArray<phx::rhi::GpuBumpAllocator, phx::rhi::MaxFramesInFlight> m_frame_allocators;
        phx::rhi::TextureAllocator m_texture_allocator;
        phx::rhi::DescriptorAllocator m_tex_descriptor_alloc;
        
        phx::renderer::HdrRenderTargets m_hdr_render_targets;
    };
}