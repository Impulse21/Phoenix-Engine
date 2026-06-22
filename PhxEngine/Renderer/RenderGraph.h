#pragma once

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/ScratchAllocator.h>
#include <PhxEngine/RHI/RHITypes.h>


namespace phx::renderer
{

    using RenderTargetRef = u32;

    enum class ResourceUsage : u8 
    { 
        Read,
        Write 
    };

    struct RenderTargetDesc
    {

        u32 width   = 0;
        u32 height  = 0;
        const char* debug_name = "";
    };

    struct PassAccess
    {
        RenderTargetRef render_target;
        ResourceUsage   usage;
    };

    using PassCallback = void(*)(rhi::CommandBufferHandle, void* user_data);

    struct PassDesc
    {
        const char*     name        = "";
        PassCallback    callback    = nullptr;
        void*           user_data   = nullptr;
        
        PassAccess* accesses        = nullptr;
        u32         access_count    = 0;
        u32         access_capacity = 0;
    };

    class PassBuilder
    {
    public:
        RenderTargetRef Read(RenderTargetRef render_target);
        RenderTargetRef Write(RenderTargetRef render_target);

    private:
        friend class RenderGraphBuilder;

        explicit PassBuilder(PassDesc* pass) 
            : m_pass(pass) 
            {}

    private:
        PassDesc m_pass;
    };

    class CompiledRenderGraph
    {
    public:
        void Execute();
    };

    class RenderGraphBuilder
    {
    public:
        explicit RenderGraphBuilder(FrameAllocator& frame_alloc, ScratchAllocator& scratch_alloc);

        RenderTargetRef DeclareRenderTarget(const RenderTargetDesc& desc);
        RenderTargetRef GetBackbuffer();

        template<typename TSetupFn>
        RenderGraphBuilder& AddPass(const char* name, PassCallback cb, void* user_data, TSetupFn&& setup)
        {
            PassDesc& pass = AllocPass(name, cb, userData);
            PassBuilder pb(&pass);
            setup(pb);
            return *this;
        }


        CompiledRenderGraph* Compile();

    private:
        PassDesc& AllocPass(const char* name, PassCallback cb, void* userData);

    private:
        FrameAllocator& m_sratch_alloc;
        ScratchAllocator& m_frame_alloc;

        PassDesc*       m_passes;

        u32             m_passCount    = 0;
        u32             m_passCapacity;
    };

}