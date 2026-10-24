#pragma once

#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/StaticArray.h>
#include <PhxEngine/Core/EnumUtils.h>

#include <PhxEngine/Memory/FrameAllocator.h>

#include <PhxEngine/Renderer/RenderGraphRegistry.h>
#include <PhxEngine/Renderer/RenderGraph.h>
#include <PhxEngine/Renderer/HdrRenderTargets.h>
#include <PhxEngine/Renderer/Renderer.h>


#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>
#include <PhxEngine/RHI/GpuMemory/DescriptorAllocator.h>

#include <PhxEngine/ECS/World.h>

#include "FrameData.h"
#include "Shaders/HordeShader_interop.h"

#include <hlsl++.h>

namespace horde
{
    struct FrameData;

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

        // TODO: Add support building  pre render Task Graph.
        void PreRender(
            const phx::ecs::World& world,
            phx::ecs::EntityId main_camera_id,
            FrameData& frame_data);

        void Render(const FrameData& frame_data);

    private:
        phx::rhi::PipelineStateHandle CreatePso(phx::Span<phx::rhi::ShaderStageInfo> shader_tages) const;

        phx::rhi::GpuBumpAllocator& GetFrameGpuAllocaor()
        {
            const usize index = phx::rhi::GetFrameIndex();
            PHX_ASSERT(index < m_frame_allocators.Size());

            return m_frame_allocators[index];
        }

    private:
        void BuildViews(
            const phx::ecs::World& world,
            phx::ecs::EntityId main_camera_id,
            phx::rhi::GpuBumpAllocator& frame_gpu_allocator,
            phx::rhi::GpuCpuRange<shader_interop::FrameConstants>& gpu_frame_constants,
            FrameData& frame_data) const;

        phx::Span<RenderPacket> BuildRenderList(
            const phx::ecs::World& world,
            phx::rhi::GpuBumpAllocator& frame_gpu_allocator,
            const FrameData& frame_data,
            phx::rhi::GpuCpuRange<shader_interop::FrameConstants> gpu_frame_constants,
            u32 view_index) const;

        [[nodiscard]] phx::rhi::UploadTicket PreloadedTextures(
            phx::rhi::GpuBumpAllocator& upload_allocator,
            phx::rhi::TextureAllocator& texture_allocator,
            phx::rhi::DescriptorAllocator& descriptor_allocator);

        struct Texture;
        bool LoadTexture(
            const char* filename,
            phx::rhi::GpuBumpAllocator& upload_allocator,
            phx::rhi::TextureAllocator& texture_allocator,
            phx::rhi::DescriptorAllocator& descriptor_allocator,
            phx::rhi::CommandBuffer cmd,
            Texture& texture);
    private:

        enum RenderViewKind : u32
        {
            Camera = 0,
            SunShadow,
        };

        phx::renderer::RenderGraphRegistry m_rg_registry;

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

        enum class PreloadedTexture : u8
        {
            BlockoutBlack = 0,
            BlockoutGrey,
            Count,
        };
        static constexpr u8 kPreloadTextureCount = static_cast<u8>(PreloadedTexture::Count);

        // TODO: Make this a resource type?
        struct Texture
        {
            phx::rhi::PlacedTexture placed_texture;
            phx::rhi::DescriptorIndex descriptor;
        };

        phx::EnumArray<Texture, PreloadedTexture> m_preloaded_textures;
        phx::rhi::UploadTicket m_texture_upload_ticket;
        bool m_is_textures_loaded = false;
    };
}