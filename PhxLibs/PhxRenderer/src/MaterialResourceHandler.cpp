#include "PhxRenderer_pch.h"
#include <PhxRenderer/MaterialResourceHandler.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/VirtualFileSystem.h>

#include <PhxAsset/AssetDatabase.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxRenderer/TextureResourceHandler.h>

#include <PhxResource/ResourceManager.h>
#include <PhxResource/IO/StreamingDefintions.h>
#include <PhxResource/IO/IoQueue.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;
using namespace phx::asset;

namespace
{
    enum InternalState
    {
        State_Init                      = ResourceState::Loading,
        State_Collect_Dependencies      = ResourceState::Loading + 1,
        State_Wait_For_Dependencies     = ResourceState::Loading + 2,
        State_Fill_Instance_Data        = ResourceState::Loading + 3,
        State_Check_Dependencies        = ResourceState::Waiting_dependencies
    };
}

LoaderStepResult MaterialResourceHandler::Step(LoadContext& ctx) const
{
    RefCountPtr<MaterialResource> mat_handle = ctx.handle.As<MaterialResource>();
    auto state = ctx.GetInternalState<InternalState>();

    switch (state)
    {
    case State_Init:
    {
        // Just block load here. If this is a baked build,
        // these should already be in memory
        mat_handle->instance_def = AssetDB::Get<asset::MaterialInstanceDef>(ctx.resource_descriptor.virtual_path.c_str());

        if (!mat_handle->instance_def)
        {
            PHX_CORE_ERROR(
                "Failed to load material instance definition from path '{0}'",
                 ctx.resource_descriptor.virtual_path.c_str());
            return LoaderStepResult::Error;
        }

        ctx.state_index++;
        return LoaderStepResult::Continue;
    }
    case State_Collect_Dependencies:
    {
        // -- Load shader module ---
        mat_handle->archetype = ResourceManager::Load<MaterialArchetypeResource>(mat_handle->instance_def->archetype.c_str());
        ctx.dependencies.push_back(mat_handle->archetype);

        // -- Load Textures --_
        for (const auto& param : mat_handle->instance_def->overrides)
        {
            std::visit([&](auto &&arg)
            {
                using T = std::decay_t<decltype(arg)>;
                
                if constexpr (std::is_same_v<T, asset::TextureField>) 
                {
                    RefCountPtr<TextureResource> texture = ResourceManager::Load<TextureResource>(arg.value().c_str());
                    mat_handle->textures.push_back(texture);

                    ctx.dependencies.push_back(texture);
                }
            }, param.value);
        }

        ctx.state_index++;
        return LoaderStepResult::Continue;
    }
    case State_Wait_For_Dependencies:
    {
        /// -- check module is loaded
        if (mat_handle->archetype->state != ResourceState::Loaded)
        {
            if (mat_handle->archetype->state == ResourceState::Error)
            {
                PHX_CORE_ERROR("Failed to load material instance due to archetype load failure.");
                return LoaderStepResult::Error;
            }

            return LoaderStepResult::Yield;
        }

        for (auto& texture : mat_handle->textures)
        {
            if (!TextureResourceHandler::RhiResourcesCreated(texture))
            {
                if (texture->state == ResourceState::Error)
                {
                    PHX_CORE_ERROR("Failed to load material archetype due to texture load failure.");
                    return LoaderStepResult::Error;
                }
                return LoaderStepResult::Yield;
            }
        }

        ctx.state_index++;
        return LoaderStepResult::Continue;
    }
    case State_Fill_Instance_Data:
    {
        // Dispatch this?
        RefCountPtr<MaterialArchetypeResource>& arch_handle = mat_handle->archetype;

        mat_handle->cpu_data_buffer = MemoryBuffer::CreateCopy(arch_handle->default_instance_data);

        // TODO: Clean this up, we do the same thing in the archetype loader.
        ShaderStructDesc material_struct_desc;
        const bool result = arch_handle->shader_module->FindStruct(kMaterialDataStructName, material_struct_desc);
        if (!result)
        {
            PHX_CORE_WARN(
                "Failed to find '{0}' struct in shader module for material archetype '{1}'",
                kMaterialDataStructName,
                ctx.resource_descriptor.virtual_path.c_str());

            ctx.state_index = State_Check_Dependencies;
            return LoaderStepResult::Continue;
        }

        TypedView<uint8_t> cpu_data_view = mat_handle->cpu_data_buffer.GetView<uint8_t>();
        for (auto &param : mat_handle->instance_def->overrides)
        {
            for (auto &field : material_struct_desc.fields)
            {
                if (field.name != phx::StringHash(param.name))
                    continue;

                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;

                    uint8_t* entry = cpu_data_view.Get() + field.offset;

                    if constexpr (std::is_same_v<T, rfl::Field<"texture", std::string>>) 
                    {
                        RefCountPtr<TextureResource> texture = ResourceManager::Get<TextureResource>(arg.value().c_str());

                        PHX_CORE_ASSERT(texture, "Failed to find loaded texture resource for material instance override.");

                        rhi::DescriptorIndex descriptor_index = rhi::GetDescriptorIndex(texture->texture_handle);
                        PHX_CORE_ASSERT(descriptor_index != rhi::cInvalidDescriptorIndex, "Failed to get valid descriptor index for material archetype default texture.");

                        std::memcpy(entry, &descriptor_index, field.size);
                    }
                    else 
                    {
                        PHX_CORE_ASSERT(field.size == sizeof(T), "Field size mismatch");
                        std::memcpy(entry, &arg.value(), field.size);
                    }
                }, param.value);
            }
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

        LoaderStepResult dep_result = PollDependenciesCompleted(ctx);
        if (dep_result == LoaderStepResult::Error)
        {
            PHX_CORE_ERROR("Failed to load material dependency.");
            return LoaderStepResult::Error;
        }

        return dep_result;
    }
    default:
    {
        throw std::runtime_error("Invalid Material loader state.");
    }
    }

    throw std::runtime_error("Invalid Material loader state.");
}