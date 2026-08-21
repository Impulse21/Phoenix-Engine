#include "RenderGraphRegistry.h"

#if false

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>

#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/RHI/RHI.h>

PHX_CVAR_INT(rg_max_transient_textures, 32, "Max cached transient textures in render graph registry");


namespace
{
    constexpr phx::Log::Channel k_log = { "RenderGraphRegistry" };
}

void phx::renderer::RenderGraphRegistry::Initialize(phx::IHeapAllocator* heap_alloc)
{
    m_heap_alloc = heap_alloc;
    m_texture_capacity = static_cast<u32>(CVar_rg_max_transient_textures.Get());
    m_textures = phx_new_array(m_heap_alloc, TextureEntry, m_texture_capacity);
    m_texture_count = 0;
}

void phx::renderer::RenderGraphRegistry::Shutdown() 
{
    for (u32 i = 0; i < m_texture_count; ++i)
    {
        rhi::DestroyTexture(m_textures[i].handle);
    }

    phx_delete(m_heap_alloc, m_textures);
    m_textures = nullptr;
    m_texture_count = 0;
    m_texture_capacity = 0;
}

void phx::renderer::RenderGraphRegistry::BeginFrame() 
{
    for (u32 i = 0; i < m_texture_count; ++i)
    {
        m_textures[i].is_used = false;
    }
}

rhi::TextureHandle phx::renderer::RenderGraphRegistry::FindOrCreateTexture(const TextureDesc& desc) const
{
    u64 hash = HashTextureDesc(desc);

    for (u32 i = 0; i < m_texture_count; ++i)
    {
        const TextureEntry& entry = m_textures[i];
        if (HashTextureDesc(entry.desc) == hash)
        {
            return true;
        }
    }

    TextureEntry* free_slot = nullptr;
    for (u32 i = 0; i < m_texture_count; ++i)
    {
        if (m_textures[i].is_used == false)
        {
            free_slot = &m_textures[i];
            break;
        }
    }

    if (free_slot == nullptr)
    {
        for (u32 i = 0; i < m_texture_capacity; ++i)
        {
            if (m_textures[i].is_used == false)
            {
                PHX_LOG_INFO(k_log, "Evicted transient texture: %s", m_textures[i].desc.debug_name);
                rhi::DestroyTexture(m_textures[i].handle);
                m_textures[i].handle = {};
                free_slot = &m_textures[i];
                break;
            }
        }
    }

    PHX_ASSERT(free_slot != nullptr && "No free slot for transient texture in render graph registry.");

    // Create the GPU resource
    rhi::TextureDescriptor rhi_desc = {};
    free_slot->handle = rhi::CreateTexture(rhi_desc);
    // TODO: I AM HERE.
    return free_slot->handle;
}

u64 phx::renderer::RenderGraphRegistry::HashTextureDesc(const TextureDesc& desc)
{
    u32 w = desc.width;
    u32 h = desc.height;

    u64 hash = (u64)w;
    hash ^= (u64)h              << 16;
    hash ^= (u64)desc.format    << 32;
    return hash;

}
#endif