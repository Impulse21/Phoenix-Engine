#include "RenderGraph.h"

#include <PhxEngine/Core/BitSet.h>
#include "RenderGraphRegistry.h"
#include <PhxEngine/RHI/RHI.h>


namespace
{
    constexpr u32 k_unused = ~0u;
    struct ResourceState
    {
        phx::renderer::Usage last_write = {};
        phx::renderer::Usage reads_since_write = {};
    };

    struct PassBarrier
    {
        phx::renderer::Usage src = {};
        phx::renderer::Usage dst = {};
    };

    struct RecordGroup 
    {
        u32 first_pass = 0;
        u32 end_pass = 0;
    };
}

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

    // 
    void RenderGraphBuilder::Execute()
    {
        // Initialize memory we need
        BitSet needed = BitSet::Create(*m_frame_allocator, m_resource_count);
        BitSet used  = BitSet::Create(*m_frame_allocator, m_resource_count);
        SpanMutable<bool> alive = m_frame_allocator->AllocSpan<bool>(m_pass_count);
        SpanMutable<u32> first_use = m_frame_allocator->AllocSpan<u32>(m_resource_count);
        SpanMutable<u32> last_use = m_frame_allocator->AllocSpan<u32>(m_resource_count);
        SpanMutable<Usage> total_usage = m_frame_allocator->AllocSpan<Usage>(m_resource_count);

        // Init memory to invalid state
        std::memset(first_use.data(), ~0u, sizeof(u32) * m_resource_count);
        std::memset(last_use.data(), 0, sizeof(u32) * m_resource_count);

        // Step 1: Cull passes that are not needed (i.e. no writes, or no reads/writes) ---
        for (u32 i = m_pass_count; i-- > 0;)
        {
            const PassDesc& pass = m_passes[i];
            bool keep_pass = false;

            // Determine if we keep a pass based on if it's imported (i.E Swap Chain) or 
            // if it writes to a resource that is needed.
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

        // TODO: I am on doing lifetimes calculations
        // -- Step 2: Calculate Lifetimes of transisent resources ---
        auto touch = [&](u32 pass, const ResourceAccess& access)
        {
            const u32 i_res = access.ref.index;
            if (first_use[i_res] == k_unused)
                first_use[i_res] == pass;

            last_use[i_res] = pass;
            total_usage[i_res] = total_usage[i_res] | access.usage; 
        };
        
        for (u32 i = 0; i < m_pass_count; ++i)
        {
            if (!alive[i])
                continue;

            const PassDesc& pass = m_passes[i];
            for (u32 iRead = 0; iRead < pass.read_count; ++iRead)
            {
                const ResourceAccess& r = pass.reads[iRead];
                touch(i, r);
            }

            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
            {
                const ResourceAccess& w = pass.writes[iWrite];
                touch(i, r);
            }
        }

        // -- Step 2: Now we know what passes are alive, we can construct the transient resources.
        m_registry->BeginFrame();
        
        for (u32 i = 0; i < m_resource_count; ++i)
        {
            ResourceEntry& resource = m_resources[i];
            if (!used.Get(i) && resource.is_imported)
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

        // -- Step 3: alias Resources ---
        struct Candidate
        {
            u32 res;
            u64 size;
            u64 align;
            u32 first;
            u32 last;
            u64 offset;
        };

        SpanMutable<Candidate> candidates = FrameAllocator.AllocSpan<Candidate>(m_resource_count);

        for (u32 i = 0; i < m_resource_count; ++i)
        {
            if (first_use[i] == k_unused || m_resources[i].is_imported)
                continue;

            const ResourceEntry& resource_entry = m_resources[i];
            if (resource_entry.desc.kind == ResourceKind::Texture)
            {
                rhi::GetTextureSizeAlign(resource_entry.desc.texture_desc);
            }
            const auto req = rhi::Get
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
