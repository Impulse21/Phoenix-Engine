#include "RenderGraph.h"

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/CVar.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::renderer;

PHX_CVAR_INT(rg_max_passes,             64, "Maximum number of passes allowed in a render graph");
PHX_CVAR_INT(rg_max_resources,          32, "Maximum number of resources allowed in a render graph");
PHX_CVAR_INT(rg_max_reads_per_pass,     32, "Maximum number of reads allowed in a single pass.");
PHX_CVAR_INT(rg_max_writes_per_pass,    32, "Maximum number of writes allowed in a single pass.");

phx::renderer::RenderGraphBuilder::RenderGraphBuilder(FrameAllocator* frame_alloc)
    : m_frame_alloc(frame_alloc)
    , m_pass_capacity(CVar_rg_max_passes.Get())
    , m_pass_count(0)
    , m_resource_count(0)
{
    m_passes = phx_frame_new_array(PassDesc, CVar_rg_max_passes.Get());
    m_resources = phx_frame_new_array(ResourceEntry, CVar_rg_max_resources.Get());
}

GraphResource phx::renderer::RenderGraphBuilder::DeclareResource(const ResourceDesc& resource_desc)
{
    PHX_ASSERT(m_resource_count < static_cast<u32>(CVar_rg_max_resources.Get() - 1));
    GraphResource resource(m_resource_count++);

    m_resources[resource.index].desc           = resource_desc;
    m_resources[resource.index].current_layout = rhi::ResourceStates::Common;
    m_resources[resource.index].external_texture = {};
    m_resources[resource.index].is_back_buffer   = false;
    m_resources[resource.index].is_imported      = false;
    return resource;
}

FramePtr<renderer::CompiledRenderGraph> phx::renderer::RenderGraphBuilder::Compile()
{
    // Marked cached RHI resources as NOT used. So we can track which resources are used in the graph and which are not.
    // BeginFrameTransients();

    FramePtr<renderer::CompiledRenderGraph> compiled_graph = phx_frame_new(renderer::CompiledRenderGraph);
    compiled_graph->m_resources = m_resources;
    compiled_graph->m_passes = m_passes;
    compiled_graph->m_resource_count = m_resource_count;
    compiled_graph->m_pass_count = m_pass_count;

    // Resolve Transient Resources and allocate them in the RHI.
    for (u32 i = 0; i < m_resource_count; ++i)
    {
        ResourceEntry& resource_entry = m_resources[i];
        if (resource_entry.is_back_buffer || resource_entry.is_imported)
            continue;

        switch (resource_entry.desc.kind)
        {
            case ResourceKind::Texture:
                resource_entry.external_texture = FindOrCreateTexture(resource_entry.desc.texture);
                resource_entry.current_layout = rhi::ResourceStates::Common;
                break;
            case ResourceKind::Buffer:

                PHX_ASSERT(false && "Gpu Buffers are not supported yet in the render graph yet.");
                break;
            default:
                PHX_ASSERT(false && "Unknown resource kind");
                break;
        }
    }
    return FramePtr<renderer::CompiledRenderGraph>();
}

phx::renderer::PassBuilder::PassBuilder(FrameAllocator&, PassDesc* desc)
    : m_desc(desc)
{
    PHX_ASSERT(desc != nullptr);
    
    // Inline maybe for optimization.
    desc->name = "";
    desc->callback = nullptr;
    desc->reads = phx_frame_new_array(Reference, CVar_rg_max_reads_per_pass.Get());
    desc->writes = phx_frame_new_array(Reference, CVar_rg_max_writes_per_pass.Get());
    desc->read_count = 0;
    desc->write_count = 0; 
}
