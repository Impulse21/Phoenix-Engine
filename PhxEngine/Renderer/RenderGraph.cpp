#include "RenderGraph.h"

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Core/CVar.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::renderer;

PHX_CVAR_INT(rg_max_passes, 64, "Maximum number of passes allowed in a render graph");
PHX_CVAR_INT(rg_max_resources, 32, "Maximum number of resources allowed in a render graph");

phx::renderer::RenderGraphBuilder::RenderGraphBuilder(FrameAllocator* frame_alloc)
    : m_frame_alloc(frame_alloc)
    , m_num_passes(0)
    , m_num_resources(0)
{
    m_passes = phx_frame_new_array(m_frame_alloc, PassDesc, rg_max_passes.Get());
    m_resources = phx_frame_new_array(m_frame_alloc, ResourceEntry, rg_max_resources.Get());
}

GraphResource phx::renderer::RenderGraphBuilder::DeclareResource(const ResourceDesc)
{
    PHX_ASSERT(m_resource_count < rg_max_resources.Get() - 1, "Exceeded maximum number of resources in render graph");
    GraphResource resource = {
        .index = m_resource_count++
    };

    m_resources[resource.index] = {
        .desc = desc,
        .state = ResourceState::Undefined
    };

    return resource;
}
