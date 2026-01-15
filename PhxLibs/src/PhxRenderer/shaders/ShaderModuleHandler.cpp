#include "PhxRenderer/PhxRenderer_pch.h"
#include "ShaderModuleHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxRenderer/shaders/SlangShaderCompiler.h>
#include <PhxRenderer/shaders/ShaderModuleResource.h>

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
    // TODO: I am here.
    RefCountPtr<ShaderModuleResource> shader_module = ctx.handle.As<ShaderModuleResource>();
    auto state = ctx.GetInternalState<InternalState>();


    switch (state)
    {
    case State_Init:
    {
        ctx.file_buffer = MemoryBuffer(ctx.resource_descriptor.length_of_resource);
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
        auto io_queue = IIoQueue::Ptr;
        if (!io_queue->IsComplete(ctx.io_ticket))
        {
            return LoaderStepResult::Yield;
        }
        auto result = io_queue->GetResult(ctx.io_ticket);
        if (result.error_code != ErrorCode::Success)
        {
            PHX_CORE_ERROR("Failed to load shader module source file.");
            return LoaderStepResult::Error;
        }

        ctx.state_index = State_Load_Shader_Module;
        return LoaderStepResult::Continue;
    }
    case State_Load_Shader_Module:
    {
        ctx.job_sync.Add();
        phx::JobSystem::SubmitJob([shader_module, ctx = &ctx](const phx::JobContext&) {

            const char* virtual_path = ctx->resource_descriptor.virtual_path.c_str();

            shader_module->slang_module = SlangShaderCompiler::LoadModule(
                ctx->file_buffer.Data(),
                ctx->file_buffer.Size(),
				GetFileNameWithoutExt(virtual_path).c_str(),
                virtual_path);

            ctx->job_sync.Signal();
        }, phx::JobSystem::Priority::Low);

        ctx.state_index = State_Wait_For_Module_Load;
        return LoaderStepResult::Continue;
    }
    case  State_Wait_For_Module_Load:
    {
        if (ctx.job_sync.IsNotCleared())
        {
            return LoaderStepResult::Yield;
        }

        return LoaderStepResult::Done;
    }
    default:
    {
        throw std::runtime_error("Invalid Shader Module loader state.");
    }
    }

    throw std::runtime_error("Invalid Shader Module loader state.");
}
