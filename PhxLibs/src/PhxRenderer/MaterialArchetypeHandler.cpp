#include "PhxRenderer/PhxRenderer_pch.h"
#include "MaterialArchetypeHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxResource/ResourceManager.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

// todo: fix this path
#include <PhxWorld/Compiler/MaterialResourceSerialization.h>

#include <nlohmann/json.hpp>
#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
    enum InternalState
    {
        State_Init = ResourceState::Loading,
        State_Wait_For_Load = ResourceState::Loading + 1,
        State_Parse_Mtl_Arch = ResourceState::Loading + 2,
        State_Wait_For_Parse = ResourceState::Loading + 3,
        State_Check_Dependencies = ResourceState::Waiting_dependencies
    };
}

LoaderStepResult MaterialArchetypeResourceHandler::Step(LoadContext& ctx) const
{
    RefCountPtr<MaterialResource> mat_handle = ctx.handle.As<MaterialResource>();
    auto state = ctx.GetInternalState<InternalState>();

    switch (state)
    {
    case State_Init:
    {
        ctx.file_buffer = MemoryBuffer(ctx.resource_descriptor.length_of_resource);
        StreamingRequest  request = {
        .debug_name = "Material Load request",
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
            PHX_CORE_ERROR("Failed to material source file.");
            return LoaderStepResult::Error;
        }

        ctx.state_index = State_Parse_Mtl_Arch;
        return LoaderStepResult::Continue;
    }
    case State_Parse_Mtl_Arch:
    {
        ctx.job_sync.Add();
        phx::JobSystem::SubmitJob([mat_handle, ctx = &ctx](const phx::JobContext&) {
            LoadMaterial(*ctx, mat_handle);
            ctx->job_sync.Signal();
            }, phx::JobSystem::Priority::Low);

        ctx.state_index = State_Wait_For_Parse;
        return LoaderStepResult::Continue;
    }
    case  State_Wait_For_Parse:
    {
        if (ctx.job_sync.IsNotCleared())
        {
            return LoaderStepResult::Yield;
        }

        if (!g_force_shallow_load && !ctx.dependencies.empty())
        {
            ctx.state_index = State_Check_Dependencies;
            return LoaderStepResult::Continue;
        }

        return LoaderStepResult::Done;
    }
    case State_Check_Dependencies:
    {
        bool all_deps_loaded = true;
        bool has_error = false;
        for (const RefCountPtr<Resource>& dep_handle : ctx.dependencies)
        {
            if (dep_handle->state == ResourceState::Error)
            {
                has_error = true;
                break;
            }

            if (dep_handle->state != ResourceState::Loaded)
            {
                all_deps_loaded = false;
                break;
            }
        }

        if (has_error)
        {
            PHX_CORE_ERROR("Failed to load material dependency.");
            return LoaderStepResult::Error;
        }

        if (!all_deps_loaded)
        {
            return LoaderStepResult::Yield;
        }

        return LoaderStepResult::Done;
    }
    default:
    {
        throw std::runtime_error("Invalid Material loader state.");
    }
    }

    throw std::runtime_error("Invalid Material loader state.");
}

void phx::renderer::MaterialArchetypeResourceHandler::LoadMaterial(LoadContext& ctx, RefCountPtr<MaterialArchetypeResource> material_resource)
{
    const char* begin = reinterpret_cast<const char*>(ctx.file_buffer.Data());
    const char* end = begin + ctx.resource_descriptor.length_of_resource;

    nlohmann::json j = nlohmann::json::parse(begin, end);
    MaterialManifest manifest = j.get<MaterialManifest>();


    PHX_CORE_WARN("Material archetypes are not setup yet.");
    material_resource->archetype = nullptr;
    material_resource->variables.reserve(manifest.properties.size());
#if false
    for (auto& [name, value] : manifest.properties)
    {

        MaterialVariable variable = {};
        variable.name = name;
        variable.value.type = value.type;
        switch (value.type)
        {
        case MaterialPropertyType::Float:
            variable.value = value.float_val;
            break;
        case MaterialPropertyType::Int:
            variable.value = value.int_val;
            break;
        case MaterialPropertyType::Bool:
            variable.value = value.bool_val;
            break;
        case MaterialPropertyType::Float2:
            variable.value = value.float2_val;
            break;
        case MaterialPropertyType::Float3:
            variable.value = value.float3_val;
            break;
        case MaterialPropertyType::Float4:
            variable.value = value.float4_val;
            break;
        case MaterialPropertyType::Texture:
            variable.value = resource_system->Get(value.texture_path.c_str());
            break;
        default:
            j = nullptr;
            break;
        }

        material_resource->variables[name] = variable;
    }
#else
    for (auto& [name, value] : manifest.properties)
    {
        MaterialVariable& variable = material_resource->variables.emplace_back();
        variable.name = name;
        variable.value.type = value.type;
        switch (value.type)
        {
        case MaterialPropertyType::Float:
            variable.value = value.float_val;
            break;
        case MaterialPropertyType::Int:
            variable.value = value.int_val;
            break;
        case MaterialPropertyType::Bool:
            variable.value = value.bool_val;
            break;
        case MaterialPropertyType::Float2:
            variable.value = value.float2_val;
            break;
        case MaterialPropertyType::Float3:
            variable.value = value.float3_val;
            break;
        case MaterialPropertyType::Float4:
            variable.value = value.float4_val;
            break;
        case MaterialPropertyType::Texture:
            variable.value.texture = ResourceManager::Load<renderer::TextureResource>(value.texture_path.c_str());
            ctx.dependencies.push_back(variable.value.texture);
            break;
        default:
            j = nullptr;
            break;
        }
    }
#endif
}
