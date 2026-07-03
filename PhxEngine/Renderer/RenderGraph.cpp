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
    m_passes = phx_frame_new_array(*m_frame_alloc, PassDesc, CVar_rg_max_passes.Get());
    m_resources = phx_frame_new_array(*m_frame_alloc, ResourceEntry, CVar_rg_max_resources.Get());
}

GraphResource phx::renderer::RenderGraphBuilder::DeclareResource(const ResourceDesc resource_desc)
{
    PHX_ASSERT(m_resource_count < CVar_rg_max_resources.Get() - 1, "Exceeded maximum number of resources in render graph");
    GraphResource resource = {
        .index = m_resource_count++
    };

    m_resources[resource.index] = {
        .desc = resource_desc,
        .state = rhi::ResourceStates::Common
    };

    return resource;
}

phx::renderer::PassBuilder::PassBuilder(FrameAllocator& frame_alloc, PassDesc* desc)
{
    // Inline maybe for optimization.
    desc->name = "";
    desc->callback = nullptr;
    desc->reads = phx_frame_new_array(frame_alloc, Reference, CVar_rg_max_reads_per_pass.Get());
    desc->writes = phx_frame_new_array(frame_alloc, Reference, CVar_rg_max_writes_per_pass.Get());
    desc->read_count = 0;
    desc->write_count = 0; 
}
