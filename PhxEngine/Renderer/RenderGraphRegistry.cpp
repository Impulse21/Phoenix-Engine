#include "RenderGraphRegistry.h"

#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Thread.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/RHI/GpuMemory/DescriptorAllocator.h>

#include <cstring>

namespace
{
    constexpr phx::Log::Channel k_log = { "RenderGraphRegistry" };
}

namespace phx::renderer
{
    void RenderGraphRegistry::Initialize(rhi::TextureHeap heap, rhi::DescriptorAllocator& descriptor_allocator)
    {
        PHX_ASSERT(heap.internal_state != nullptr && heap.size > 0);
        PHX_ASSERT(m_descriptor_allocator == nullptr && "RenderGraphRegistry initialized twice");

        m_heap                 = heap;
        m_descriptor_allocator = &descriptor_allocator;
        m_placement.Initialize(heap.size);
        m_texture_count        = 0;
    }

    void RenderGraphRegistry::Shutdown()
    {
        if (m_descriptor_allocator == nullptr)
            return;

        // Both destroys are deferred by the RHI until the GPU is done.
        while (m_texture_count > 0)
            Evict(m_texture_count - 1);

        m_heap                 = {};
        m_descriptor_allocator = nullptr;
        m_placement            = {};
    }

    void RenderGraphRegistry::BeginFrame()
    {
        m_placement.Reset();
        for (u32 i = 0; i < m_texture_count; ++i)
            m_textures[i].is_used = false;
    }

    TransientTexture RenderGraphRegistry::Acquire(const rhi::TextureDescriptor& desc)
    {
        PHX_ASSERT(Thread::IsMainThread());
        PHX_ASSERT(m_descriptor_allocator != nullptr && "RenderGraphRegistry not initialized");

        // -- Place ---
        const rhi::SizeAlign size_align = rhi::GetTextureSizeAlign(desc);
        const u64 offset = m_placement.Alloc(size_align.size, size_align.align);
        if (offset == BumpCore::kInvalidOffset)
        {
            PHX_LOG_ERROR(k_log,
                "Transient texture heap full placing '{}': needs {} bytes (align {}), {} of {} bytes already used this frame",
                desc.debug_name, size_align.size, size_align.align, m_placement.Used(), m_placement.Capacity());
            PHX_ASSERT(false && "Transient texture heap full; raise its size");
            return {};
        }

        // -- Cached? ---
        for (u32 i = 0; i < m_texture_count; ++i)
        {
            TextureEntry& entry = m_textures[i];
            if (!entry.is_used && entry.offset == offset && IsCompatible(entry.desc, desc))
            {
                entry.is_used = true;
                return { .handle = entry.handle, .descriptor = entry.descriptor };
            }
        }

        // -- Create ---
        // Unused entries are only evicted at EndFrame(), so a frame where the
        // graph changes can briefly hold last frame's entries plus this frame's.
        if (m_texture_count >= k_max_transient_textures)
        {
            PHX_LOG_ERROR(k_log, "Transient texture cache full ({} entries) creating '{}'",
                k_max_transient_textures, desc.debug_name);
            PHX_ASSERT(false && "Transient texture cache full; raise k_max_transient_textures");
            return {};
        }

        TextureEntry& entry = m_textures[m_texture_count++];
        entry.desc       = desc;
        entry.offset     = offset;
        entry.is_used    = true;
        entry.handle     = rhi::CreateTexture(desc, m_heap, offset);
        entry.descriptor = EnumHasAnyFlags(desc.binding_flags, rhi::BindingFlags::ShaderResource)
            ? m_descriptor_allocator->Allocate(entry.handle)
            : rhi::kInvalidDescriptorIndex;

        PHX_ASSERT(entry.handle.IsValid());
        return { .handle = entry.handle, .descriptor = entry.descriptor };
    }

    void RenderGraphRegistry::EndFrame()
    {
        // Backwards so swap-remove never skips an entry.
        for (u32 i = m_texture_count; i-- > 0;)
        {
            if (!m_textures[i].is_used)
                Evict(i);
        }
    }

    bool RenderGraphRegistry::IsCompatible(const rhi::TextureDescriptor& a, const rhi::TextureDescriptor& b)
    {
        // Everything fixed at texture creation. Ignored on purpose:
        //   debug_name    - cosmetic
        //   initial_state - only applies at creation; afterwards the graph
        //                   owns the texture's state
        return a.texture_type  == b.texture_type
            && a.format        == b.format
            && a.width         == b.width
            && a.height        == b.height
            && a.array_size    == b.array_size
            && a.mip_levels    == b.mip_levels
            && a.sample_count  == b.sample_count
            && a.usage         == b.usage
            && a.binding_flags == b.binding_flags
            && a.misc_flags    == b.misc_flags
            && std::memcmp(&a.clear_value, &b.clear_value, sizeof(rhi::ClearValue)) == 0;
    }

    void RenderGraphRegistry::Evict(u32 index)
    {
        PHX_ASSERT(index < m_texture_count);
        TextureEntry& entry = m_textures[index];

        // Both are deferred until the GPU has finished with them.
        rhi::DestroyTexture(entry.handle);
        m_descriptor_allocator->Free(entry.descriptor);   // No-op for kInvalidDescriptorIndex.

        // Swap-remove: order doesn't matter, lookup is by (desc, offset).
        m_textures[index] = m_textures[--m_texture_count];
    }
}
