#include "RenderGraph.h"

#include <PhxEngine/Core/BitSet.h>
#include "RenderGraphRegistry.h"
#include <PhxEngine/RHI/RHI.h>

// -- Type declerations ---
namespace phx::renderer
{
    struct ResourceEntry
    {
        ResourceDesc desc;
        union
        {
            rhi::TextureHandle external_texture;
            rhi::GpuRange      external_gpu_buffer;
        };

        bool is_imported    = false;
        bool is_back_buffer = false;

        ResourceEntry()
            : desc{}
            , external_texture{}
        {
        }
    };

    RenderGraphBuilder::RenderGraphBuilder(
        RenderGraphRegistry& registry, FrameAllocator& frame_allocator, const RenderGraphLimits& limits)
        : m_registry(&registry)
        , m_frame_allocator(&frame_allocator)
        , m_resource_capacity(limits.max_resources)
        , m_pass_capacity(limits.max_passes)
    {
        m_passes    = frame_allocator.Alloc<PassDesc>(m_pass_capacity);
        m_resources = frame_allocator.Alloc<ResourceEntry>(m_resource_capacity);

        auto reads  = frame_allocator.Alloc<ResourceAccess>(m_pass_capacity * limits.max_reads_per_pass);
        auto writes = frame_allocator.Alloc<ResourceAccess>(m_pass_capacity * limits.max_writes_per_pass);

        for (u32 i = 0; i < m_pass_capacity; ++i)
        {
            PassDesc& pass      = m_passes[i];
            pass.name           = nullptr;
            pass.reads          = reads + i * limits.max_reads_per_pass;
            pass.writes         = writes + i * limits.max_writes_per_pass;
            pass.read_capacity  = limits.max_reads_per_pass;
            pass.write_capacity = limits.max_writes_per_pass;
        }
    }

    void RenderGraphBuilder::Execute(ScratchAllocator& scratch_allocator)
    {
        // Pass in scratch buffer?
        BitSet needed = BitSet::Create(scratch_allocator, m_resource_count);
        BitSet used  = BitSet::Create(scratch_allocator, m_resource_count);
        bool* alive = scratch_allocator.Alloc<bool>(m_pass_count);

        // Step 1: Cull passes that are not needed (i.e. no writes, or no reads/writes)
        for (u32 i = m_pass_count; i-- > 0;)
        {
            const PassDesc& pass = m_passes[i];
            bool keep_pass = false;

            // Determine if a pass is a root (i.e we write to an imported resource, or someone reads it)
            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
            {
                const ResourceAccess& w = pass.writes[iWrite];
                keep_pass |= m_resources[w.ref.index].is_imported || needed.Get(w.ref.index);
            }

            alive[i] = keep_pass;
            if (!keep_pass)
                continue;

            // Mark all reads as needed
            for (u32 iRead = 0; iRead < pass.read_count; ++iRead)
            {
                const ResourceAccess& r = pass.reads[iRead];
                needed.Set(r.ref.index, true);
            }

            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
            {
                const ResourceAccess& w = pass.writes[iWrite];
                used.Set(w.ref.index, true);
            }
        }

        // Now we know what passes are alive, we can construct the transient resources.
        m_registry->BeginFrame();
        for (u32 i = 0; i < m_resource_count; ++i)
        {
            if (!used.Get(i))
                continue;

            ResourceEntry& resource = m_resources[i];
            if (resource.is_imported)
                continue;

            // Create transient resources. Would be good to figure out how to alias resource that are not alive at the same time.
            switch (resource.desc.kind)
            {
                case ResourceKind::Texture:
                    break;
                case ResourceKind::Buffer:
                    break; 
            };
        }

        // -- Forward sweep to determine barriers

        // -- Execute passes ---
        // in the future, do a cmd per pass, or merge trival passes into a single cmd.
        rhi::CommandBuffer cmd = rhi::BeginCommandRecording(rhi::CommandQueueType::Graphics);
        for (u32 i = 0; i < m_pass_count; ++i)
        {
            if (!alive[i])
                continue;

            PassDesc& pass = m_passes[i];

            // TODO: Need to figure htis out.
            PassContext pass_context;
            pass.execute_fn(cmd, pass_context);
        }

        scratch_allocator.Reset();
    }

    GraphResource RenderGraphBuilder::DeclareResource(const ResourceDesc& resource_desc)
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity- 1);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc           = resource_desc;
        m_resources[resource.index].external_texture = {};
        m_resources[resource.index].is_back_buffer   = false;
        m_resources[resource.index].is_imported      = false;

        return resource;
    }

    GraphResource RenderGraphBuilder::ImportTexture(rhi::TextureHandle texture)
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity- 1);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc                =  { .kind = ResourceKind::Texture, .buffer_desc = {} };
        m_resources[resource.index].external_texture    = texture;
        m_resources[resource.index].is_back_buffer      = false;
        m_resources[resource.index].is_imported         = true;

        return resource;
    }

    GraphResource RenderGraphBuilder::ImportBuffer(rhi::GpuRange buffer)
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity- 1);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc                =  { .kind = ResourceKind::Buffer, .buffer_desc = {} };
        m_resources[resource.index].external_gpu_buffer = buffer;
        m_resources[resource.index].is_back_buffer      = false;
        m_resources[resource.index].is_imported         = true;

        return resource;
    }

    GraphResource RenderGraphBuilder::ImportSwaphchain()
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity- 1);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc                = {};
        m_resources[resource.index].external_gpu_buffer = {};
        m_resources[resource.index].is_back_buffer      = true;
        m_resources[resource.index].is_imported         = true;

        return resource;
    }

    PassBuilder RenderGraphBuilder::BeginPass(const char* pass_name)
    {
        PHX_ASSERT(m_pass_count < m_pass_capacity - 1);
        
        PassDesc& pass      = m_passes[m_pass_count++];
        pass.name           = pass_name;
		pass.read_count     = 0;
        pass.write_count    = 0;

        return PassBuilder(pass);
    }
}  // namespace phx::renderer
