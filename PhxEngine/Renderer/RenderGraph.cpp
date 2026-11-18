#include "RenderGraph.h"

#include <PhxEngine/Core/BitSet.h>
#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Memory/MemoryHelpers.h>
#include "RenderGraphRegistry.h"
#include <PhxEngine/RHI/RHI.h>

#include <algorithm>


namespace
{
    using phx::renderer::Usage;
    using phx::rhi::BarrierAccess;
    using phx::rhi::BarrierStage;

    constexpr u32 k_unused = ~0u;
    constexpr Usage k_unknown_prior_use = {
        BarrierStage::AllCommands,
        BarrierAccess::ColorWrite | BarrierAccess::DepthStencilWrite | BarrierAccess::ShaderWrite | BarrierAccess::TransferWrite,
    };

    // Per-resource state for the forward barrier sweep.
    struct ResourceState
    {
        Usage last_write        = {};
        Usage reads_since_write = {};    // Readers that already waited on last_write.
        bool  touched           = false; // Used by an earlier pass this frame.
    };

    // One merged barrier per pass: the OR of every resource's dependency.
    struct PassBarrier
    {
        Usage src = {};
        Usage dst = {};
    };

    struct RecordGroup
    {
        u32 first_pass = 0;
        u32 end_pass = 0;
    };

    bool IsEmpty(Usage usage)
    {
        return usage.stage == BarrierStage::None;
    }

    // True if `covered` already includes every stage and access in `usage`.
    bool Covers(Usage covered, Usage usage)
    {
        return phx::EnumHasAllFlags(covered.stage, usage.stage)
            && phx::EnumHasAllFlags(covered.access, usage.access);
    }

    // Adds the dependency one access needs to the pass barrier. `state` is the
    // resource's state *before* this pass, so a pass never waits on itself.
    void AccumulateBarrier(const ResourceState& state, Usage usage, bool is_write, PassBarrier& barrier)
    {
        if (!state.touched)
        {
            barrier.src = barrier.src | k_unknown_prior_use;
            barrier.dst = barrier.dst | usage;
            return;
        }

        if (is_write)
        {
            if (!IsEmpty(state.reads_since_write))
            {
                // Write-after-read: the readers must finish first. Nothing to
                // make visible, so it's an execution dependency only. Any
                // earlier write is already ordered through those readers.
                barrier.src.stage |= state.reads_since_write.stage;
                barrier.dst = barrier.dst | usage;
            }
            else if (!IsEmpty(state.last_write))
            {
                // Write-after-write.
                barrier.src = barrier.src | state.last_write;
                barrier.dst = barrier.dst | usage;
            }
        }
        else
        {
            // Read-after-write, unless an earlier reader already waited on the
            // same write for this stage/access. Read-after-read needs nothing.
            if (!IsEmpty(state.last_write) && !Covers(state.reads_since_write, usage))
            {
                barrier.src = barrier.src | state.last_write;
                barrier.dst = barrier.dst | usage;
            }
        }
    }

    phx::rhi::BindingFlags BindingFlagsFromAccess(BarrierAccess access)
    {
        using phx::EnumHasAnyFlags;
        using phx::rhi::BindingFlags;

        BindingFlags flags = BindingFlags::None;
        if (EnumHasAnyFlags(access, BarrierAccess::ColorRead | BarrierAccess::ColorWrite))
            flags |= BindingFlags::RenderTarget;
        if (EnumHasAnyFlags(access, BarrierAccess::DepthStencilRead | BarrierAccess::DepthStencilWrite))
            flags |= BindingFlags::DepthStencil;
        if (EnumHasAnyFlags(access, BarrierAccess::ShaderRead))
            flags |= BindingFlags::ShaderResource;
        if (EnumHasAnyFlags(access, BarrierAccess::ShaderWrite))
            flags |= BindingFlags::UnorderedAccess;

        return flags;
    }

    phx::rhi::ResourceStates InitialStateFromAccess(BarrierAccess first_access)
    {
        using phx::EnumHasAnyFlags;
        using phx::rhi::ResourceStates;

        if (EnumHasAnyFlags(first_access, BarrierAccess::ColorWrite))
            return ResourceStates::RenderTarget;
        if (EnumHasAnyFlags(first_access, BarrierAccess::DepthStencilWrite))
            return ResourceStates::DepthWrite;
        if (EnumHasAnyFlags(first_access, BarrierAccess::ShaderWrite))
            return ResourceStates::UnorderedAccess;

        return ResourceStates::ShaderResource;
    }

    phx::rhi::TextureDescriptor ToTextureDescriptor(const phx::renderer::TextureDesc& desc, Usage total_usage, Usage first_usage)
    {
        return {
            .debug_name     = desc.debug_name,
            .texture_type   = phx::rhi::TextureType::Texture2D,
            .format         = desc.format,
            .width          = desc.width,
            .height         = desc.height,
            .binding_flags  = BindingFlagsFromAccess(total_usage.access),
            .initial_state  = InitialStateFromAccess(first_usage.access),
        };
    }
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
        rhi::DescriptorIndex external_descriptor = rhi::kInvalidDescriptorIndex;

        bool is_imported    = false;
        bool is_back_buffer = false;

        ResourceEntry()
            : desc{}
            , external_texture{}
        {
        }
    };

    RenderGraphBuilder::RenderGraphBuilder(
        RenderGraphRegistry& registry, const RenderGraphLimits& limits)
        : m_registry(&registry)
        , m_resource_capacity(limits.max_resources)
        , m_pass_capacity(limits.max_passes)
    {
        FrameAllocator& frame_allocator = Memory::GetFrameAlloc();
        m_passes    = frame_allocator.Alloc<PassDesc>(m_pass_capacity);
        m_resources = frame_allocator.Alloc<ResourceEntry>(m_resource_capacity);

        auto reads  = frame_allocator.Alloc<ResourceAccess>(m_pass_capacity * limits.max_reads_per_pass);
        auto writes = frame_allocator.Alloc<ResourceAccess>(m_pass_capacity * limits.max_writes_per_pass);

        for (u32 i = 0; i < m_pass_capacity; ++i)
        {
            PassDesc& pass      = m_passes[i];
            pass.reads          = reads + i * limits.max_reads_per_pass;
            pass.writes         = writes + i * limits.max_writes_per_pass;
            pass.read_capacity  = limits.max_reads_per_pass;
            pass.write_capacity = limits.max_writes_per_pass;
        }
    }

    rhi::CommandBuffer RenderGraphBuilder::Execute(rhi::GpuRange texture_descriptor_heap, rhi::GpuRange sampler_descriptor_heap)
    {
        // Working memory for this Execute. FrameArray value-initialises (zeroes).
        BitSet              needed      = BitSet::Create(Memory::GetFrameAlloc(), m_resource_count);
        SpanMutable<bool>   alive       = Memory::FrameArray<bool>(m_pass_count);
        SpanMutable<u32>    first_use   = Memory::FrameArray<u32>(m_resource_count);
        SpanMutable<u32>    last_use    = Memory::FrameArray<u32>(m_resource_count);    // For aliasing later.
        SpanMutable<Usage>  first_usage = Memory::FrameArray<Usage>(m_resource_count);  // Usage in the first live pass.
        SpanMutable<Usage>  total_usage = Memory::FrameArray<Usage>(m_resource_count);
        std::fill_n(first_use.data(), m_resource_count, k_unused);

        // -- Step 1: Cull ---
        // Walk backwards. A pass is kept if it writes an imported resource
        // (i.e. the swapchain) or a resource a later kept pass reads.
        // Conservative: there is no resource versioning, so an earlier writer
        // of a resource that is later fully overwritten is still kept.
        for (u32 i = m_pass_count; i-- > 0;)
        {
            const PassDesc& pass = m_passes[i];
            bool keep_pass = false;

            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
            {
                const ResourceAccess& w = pass.writes[iWrite];
                keep_pass |= m_resources[w.ref.index].is_imported || needed.Get(w.ref.index);
            }

            alive[i] = keep_pass;
            if (!keep_pass)
                continue;

            for (u32 iRead = 0; iRead < pass.read_count; ++iRead)
            {
                const ResourceAccess& r = pass.reads[iRead];
                needed.Set(r.ref.index, true);
            }
        }

        // -- Step 2: Lifetimes and accumulated usage ---
        for (u32 i = 0; i < m_pass_count; ++i)
        {
            if (!alive[i])
                continue;

            auto touch = [&](const ResourceAccess& access)
            {
                const u32 i_res = access.ref.index;
                if (first_use[i_res] == k_unused)
                    first_use[i_res] = i;

                if (first_use[i_res] == i)
                    first_usage[i_res] = first_usage[i_res] | access.usage;

                last_use[i_res]    = i;
                total_usage[i_res] = total_usage[i_res] | access.usage;
            };

            const PassDesc& pass = m_passes[i];
            for (u32 iRead = 0; iRead < pass.read_count; ++iRead)
                touch(pass.reads[iRead]);

            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
                touch(pass.writes[iWrite]);
        }

        // -- Step 3: Resolve physical resources ---
        SpanMutable<PhysicalResource> physical = Memory::FrameArray<PhysicalResource>(m_resource_count);

        m_registry->BeginFrame();
        for (u32 i = 0; i < m_resource_count; ++i)
        {
            if (first_use[i] == k_unused)
                continue;   // No live pass touches it.

            const ResourceEntry& entry = m_resources[i];
            PhysicalResource& resolved = physical[i];

            if (entry.is_back_buffer)
            {
                resolved.is_back_buffer = true;
                continue;
            }

            if (entry.is_imported)
            {
                if (entry.desc.kind == ResourceKind::Texture)
                {
                    resolved.texture    = entry.external_texture;
                    resolved.descriptor = entry.external_descriptor;
                }
                else
                {
                    resolved.buffer = entry.external_gpu_buffer;
                }
                continue;
            }

            // Transient texture. Its contents are undefined until written, and
            // the RHI only prepares a new texture for use on its first
            // CmdBeginRenderPass / CmdCopyMemoryToTexture -- so v1 requires the
            // first use to be a render-target or depth write.
            PHX_ASSERT(EnumHasAnyFlags(first_usage[i].access, rhi::BarrierAccess::ColorWrite | rhi::BarrierAccess::DepthStencilWrite)
                && "Transient texture's first use must be a ColorTarget or DepthTarget write");

            const rhi::TextureDescriptor texture_desc =
                ToTextureDescriptor(entry.desc.texture_desc, total_usage[i], first_usage[i]);

            const TransientTexture transient = m_registry->Acquire(texture_desc);
            resolved.texture    = transient.handle;
            resolved.descriptor = transient.descriptor;
        }

        // -- Step 4: Record ---
        // One command buffer for the whole graph for now. In the future, a
        // cmd per pass group, using PassDesc::is_trivial as a hint.
        rhi::CommandBuffer cmd = rhi::BeginCommandRecording(rhi::CommandQueueType::Graphics);
        rhi::CmdSetDescriptorHeaps(cmd, texture_descriptor_heap, sampler_descriptor_heap);

        SpanMutable<ResourceState> states = Memory::FrameArray<ResourceState>(m_resource_count);
        PassContext pass_context(physical.data(), m_resource_count);

        for (u32 i = 0; i < m_pass_count; ++i)
        {
            if (!alive[i])
                continue;

            const PassDesc& pass = m_passes[i];

            // Barrier against the state before this pass.
            PassBarrier barrier = {};
            for (u32 iRead = 0; iRead < pass.read_count; ++iRead)
            {
                const ResourceAccess& r = pass.reads[iRead];
                AccumulateBarrier(states[r.ref.index], r.usage, false, barrier);
            }

            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
            {
                const ResourceAccess& w = pass.writes[iWrite];
                AccumulateBarrier(states[w.ref.index], w.usage, true, barrier);
            }

            if (!IsEmpty(barrier.src))
                rhi::CmdBarrier(cmd, barrier.src.stage, barrier.src.access, barrier.dst.stage, barrier.dst.access);

            // Advance state. Reads first, so a pass that reads and writes the
            // same resource ends with that write as the last write.
            for (u32 iRead = 0; iRead < pass.read_count; ++iRead)
            {
                const ResourceAccess& r = pass.reads[iRead];
                ResourceState& state = states[r.ref.index];
                state.touched           = true;
                state.reads_since_write = state.reads_since_write | r.usage;
            }

            for (u32 iWrite = 0; iWrite < pass.write_count; ++iWrite)
            {
                const ResourceAccess& w = pass.writes[iWrite];
                ResourceState& state = states[w.ref.index];
                state.touched           = true;
                state.last_write        = w.usage;
                state.reads_since_write = {};
            }

            pass.execute_fn(cmd, pass_context);
        }

        m_registry->EndFrame();
        return cmd;
    }

    GraphResource RenderGraphBuilder::DeclareResource(const ResourceDesc& resource_desc)
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity);
        PHX_ASSERT(resource_desc.kind == ResourceKind::Texture && "Transient buffers are not supported yet; import the buffer instead");
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc           = resource_desc;
        m_resources[resource.index].external_texture = {};
        m_resources[resource.index].is_back_buffer   = false;
        m_resources[resource.index].is_imported      = false;

        return resource;
    }

    GraphResource RenderGraphBuilder::ImportTexture(rhi::TextureHandle texture, rhi::DescriptorIndex descriptor)
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc                =  { .kind = ResourceKind::Texture, .texture_desc = {} };
        m_resources[resource.index].external_texture    = texture;
        m_resources[resource.index].external_descriptor = descriptor;
        m_resources[resource.index].is_back_buffer      = false;
        m_resources[resource.index].is_imported         = true;

        return resource;
    }

    GraphResource RenderGraphBuilder::ImportBuffer(rhi::GpuRange buffer)
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc                =  { .kind = ResourceKind::Buffer, .buffer_desc = {} };
        m_resources[resource.index].external_gpu_buffer = buffer;
        m_resources[resource.index].is_back_buffer      = false;
        m_resources[resource.index].is_imported         = true;

        return resource;
    }

    GraphResource RenderGraphBuilder::ImportSwapchain()
    {
        PHX_ASSERT(m_resource_count < m_resource_capacity);
        GraphResource resource(m_resource_count++);

        m_resources[resource.index].desc                = { .kind = ResourceKind::Texture, .texture_desc = {} };
        m_resources[resource.index].external_texture    = {};   // No handle: passes use the viewport overloads (PassContext::IsBackBuffer).
        m_resources[resource.index].is_back_buffer      = true;
        m_resources[resource.index].is_imported         = true;

        return resource;
    }

    PassBuilder RenderGraphBuilder::BeginPass(const char* pass_name)
    {
        PHX_ASSERT(m_pass_count < m_pass_capacity);

        PassDesc& pass      = m_passes[m_pass_count++];
        pass.name           = pass_name;
		pass.read_count     = 0;
        pass.write_count    = 0;

        return PassBuilder(pass);
    }
}  // namespace phx::renderer
