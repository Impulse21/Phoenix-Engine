#include "PhxRenderer_pch.h"
#include <PhxRenderer/Shaders/ShaderModuleHandler.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include "SlangShaderCompiler.h"
#include <PhxRenderer/Shaders/ShaderModuleResource.h>

using namespace phx;
using namespace phx::renderer;


namespace
{
    enum InternalState
    {
        State_Init                  = ResourceState::Loading,
        State_Wait_For_Load         = ResourceState::Loading + 1,
        State_Load_Shader_Module    = ResourceState::Loading + 2,
        State_Wait_For_Module_Load  = ResourceState::Loading + 3,
    };
}

LoaderStepResult phx::renderer::ShaderModuleHandler::Step(LoadContext& ctx) const
{
    RefCountPtr<ShaderModuleResource> shader_module = ctx.handle.As<ShaderModuleResource>();
    auto state = ctx.GetInternalState<InternalState>();

    switch (state)
    {
    case State_Init:
    {
        ctx.file_buffer = MemoryBuffer(ctx.resource_descriptor.length_of_resource,std::byte('\0'));

        StreamingRequest  request = {
            .debug_name = "Shader module load request",
            .operations = {
                {
                    .source = {
                        .data = ctx.resource_descriptor,
                        .size = ctx.resource_descriptor.length_of_resource,
                    },
                    .destination = {
                        .target = CpuDestination{.address = ctx.file_buffer.Data() },
                        .size = ctx.resource_descriptor.length_of_resource,
                    }
                }
            }
        };

        ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(request));
        ctx.state_index = State_Wait_For_Load;

        return LoaderStepResult::Continue;
    }
    case State_Wait_For_Load:
    {
        return PollQueueTask(
            ctx,
            IIoQueue::Ptr,
            [&ctx](){
                ctx.state_index = State_Load_Shader_Module;
            },
            []() {
                PHX_CORE_ERROR("Failed to load shader module source file.");
            });
    }
    case State_Load_Shader_Module:
    {
        ctx.job_sync.Add();
        phx::TaskScheduler::Submit([shader_module, ctx = &ctx]() 
        {
            const char* virtual_path = ctx->resource_descriptor.virtual_path.c_str();
            SlangCompilerSession* session = SlangCompiler::GetOrCreateCompilerSession();
            
            session->LoadModule(
                GetFileNameWithoutExt(virtual_path),
                virtual_path,
                ctx->file_buffer.Data(),
                ctx->file_buffer.Size());

            // TODO: Fine a way to handle this barrier trigger witin the
            // Scheduler.
            ctx->job_sync.Signal();
        },
        ctx.thread_pool_handle,
        phx::TaskScheduler::Priority::Low);

        ctx.state_index = State_Wait_For_Module_Load;
        return LoaderStepResult::Continue;
    }
    case  State_Wait_For_Module_Load:
    {
        return PollBarrierTask(ctx);
    }
    default:
    {
        throw std::runtime_error("Invalid Shader Module loader state.");
    }
    }

    throw std::runtime_error("Invalid Shader Module loader state.");
}
