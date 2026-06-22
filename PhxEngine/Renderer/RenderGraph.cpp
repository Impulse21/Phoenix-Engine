#include "RenderGraph.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::renderer;

RenderTargetRef phx::renderer::PassBuilder::Read(RenderTargetRef render_target)
{
    return RenderTargetRef();
}

RenderTargetRef phx::renderer::PassBuilder::Write(RenderTargetRef render_target)
{
    return RenderTargetRef();
}

phx::renderer::RenderGraphBuilder::RenderGraphBuilder(
    FrameAllocator& frame_alloc, ScratchAllocator& scratch_alloc)
{
}

RenderTargetRef phx::renderer::RenderGraphBuilder::DeclareRenderTarget(
    const RenderTargetDesc& desc)
{
    return RenderTargetRef();
}

RenderTargetRef phx::renderer::RenderGraphBuilder::GetBackbuffer()
{
    return RenderTargetRef();
}

CompiledRenderGraph* phx::renderer::RenderGraphBuilder::Compile()
{
    return nullptr;
}

PassDesc& phx::renderer::RenderGraphBuilder::AllocPass(const char* name,
                                                       PassCallback cb,
                                                       void* userData)
{
    // TODO: insert return statement here
}
