#include "PhxRenderer_pch.h"
#include <PhxRenderer/MaterialArchetypeHandler.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxAsset/AssetDatabase.h>

#include <PhxResource/ResourceManager.h>
#include <PhxResource/IO/StreamingDefintions.h>
#include <PhxResource/IO/IoQueue.h>

#include <PhxRenderer/MaterialArchetype.def.h>
#include <PhxRenderer/TextureResource.h>
#include <PhxRenderer/TextureResourceHandler.h>


#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::asset;
using namespace phx::renderer;
using namespace phx::renderer::asset;

namespace
{

    enum InternalState
    {
        State_Init                          = ResourceState::Loading,
        State_Collect_Dependencies          = ResourceState::Loading + 1,
        State_Wait_For_Dependencies         = ResourceState::Loading + 2,
        State_Fill_Instance_Data            = ResourceState::Loading + 3,
        State_Check_Dependencies            = ResourceState::Waiting_dependencies
    };
}

LoaderStepResult MaterialArchetypeResourceHandler::Step(LoadContext& ctx) const
{
    RefCountPtr<MaterialArchetypeResource> arch_handle = ctx.handle.As<MaterialArchetypeResource>();
    auto state = ctx.GetInternalState<InternalState>();

    switch (state)
    {
    case State_Init:
    {
        // Just block load here. If this is a baked build,
        // these should already be in memory
        arch_handle->archetype_def = AssetDB::Get<asset::MaterialArchetypeDef>(ctx.resource_descriptor.virtual_path.c_str());

        if (!arch_handle->archetype_def)
        {
            PHX_CORE_ERROR(
                "Failed to load material archetype definition from path '{0}'",
                 ctx.resource_descriptor.virtual_path.c_str());
            return LoaderStepResult::Error;
        }

        ctx.state_index++;
        return LoaderStepResult::Continue;
    }
    case State_Collect_Dependencies:
    {
        // -- Load shader module ---
        arch_handle->shader_module = ResourceManager::Load<ShaderModuleResource>(arch_handle->archetype_def->shader_desc.source.c_str());
        ctx.dependencies.push_back(arch_handle->shader_module);

        // -- Load Textures --_
        for (const auto& param : arch_handle->archetype_def->params)
        {
            std::visit([&](auto &&arg)
            {
                using T = std::decay_t<decltype(arg)>;
                
                if constexpr (std::is_same_v<T, asset::TextureField>) 
                {
                    RefCountPtr<TextureResource> texture = ResourceManager::Load<TextureResource>(arg.value().c_str());

                    arch_handle->texture_lut[phx::StringHash(param.name.c_str())] = arch_handle->textures.size();
                    arch_handle->textures.push_back(texture);

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
        if (arch_handle->shader_module->state != ResourceState::Loaded)
        {
            if (arch_handle->shader_module->state == ResourceState::Error)
            {
                PHX_CORE_ERROR("Failed to load material archetype due to shader module load failure.");
                return LoaderStepResult::Error;
            }

            return LoaderStepResult::Yield;
        }

        for (auto& texture : arch_handle->textures)
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

        // Create default instance size and fill 0's.
        // Even if we fail to find anything we can make a black material for the renderer
        // rather than crashing
        arch_handle->default_instance_data = phx::MemoryBuffer(kMaterialDataStructSize, std::byte(0));

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

        PHX_ASSERT(material_struct_desc.size == kMaterialDataStructSize, "MaterialData struct must be 256 bytes in size");

        TypedView<uint8_t> instance_data_view = arch_handle->default_instance_data.GetView<uint8_t>();

        const asset::MaterialArchetypeDef& arch_def = *arch_handle->archetype_def;
        for (auto &param : arch_def.params)
        {
            for (auto &field : material_struct_desc.fields)
            {
                if (field.name != phx::StringHash(param.name))
                    continue;

                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;

                    uint8_t* entry = instance_data_view.Get() + field.offset;

                    if constexpr (std::is_same_v<T, rfl::Field<"texture", std::string>>) 
                    {
                        size_t texture_index = arch_handle->texture_lut[phx::StringHash(param.name.c_str())];
                        PHX_CORE_ASSERT(texture_index < arch_handle->textures.size(), "Texture index out of bounds for material archetype default texture assignment.");
                        RefCountPtr<TextureResource>& texture = arch_handle->textures[texture_index];

                        rhi::DescriptorIndex descriptor_index = rhi::GetDescriptorIndex(texture->texture_handle);
                        PHX_CORE_ASSERT(descriptor_index != rhi::cInvalidDescriptorIndex, "Failed to get valid descriptor index for material archetype default texture.");

                        std::memcpy(entry, &descriptor_index, field.size);
                    }
                    else 
                    {
                        PHX_CORE_ASSERT(field.size == sizeof(T::Type), "Field size mismatch");
                        std::memcpy(entry, &arg.value(), field.size);
                    }
                }, param.value);
            }
        }

        ctx.state_index = State_Check_Dependencies;
        return LoaderStepResult::Continue;
    }
    case State_Check_Dependencies:
    {
        LoaderStepResult dep_result = PollDependenciesCompleted(ctx);
        if (dep_result == LoaderStepResult::Error)
        {
            PHX_CORE_ERROR("Failed to load material archetype due to dependency load failure.");
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

bool phx::renderer::MaterialArchetypeResourceHandler::LoadArchetype(LoadContext& /*ctx*/, RefCountPtr<MaterialArchetypeResource> /*arch_res*/)
{
    #if false
    const char* begin = reinterpret_cast<const char*>(ctx.file_buffer.Data());
    const char* end = begin + ctx.resource_descriptor.length_of_resource;

    // 1. Load File
    YAML::Node root;
    root = YAML::LoadFile(ctx.resource_descriptor.os_path_or_pak_path);

    if (!root["asset_type"] || root["asset_type"].as<std::string>() != "MaterialArchetype") 
    {
        return false;
    }

    if (YAML::Node technique_node = root["techniques"])
    {
        for (const auto& technique_itr : technique_node)
        {
            std::string pass_name = technique_itr.first.as<std::string>();
            YAML::Node stages_node = technique_itr.second;                 // The map of stages

            // Iterate over the keys inside the technique (e.g., "vertex", "fragment")
            for (const auto& stage_itr : stages_node)
            {
                std::string stage_key = stage_itr.first.as<std::string>();
                std::string entryPoint = stage_itr.second.as<std::string>();

                // Convert string key to Enum
                rhi::ShaderStage stage_enum = ParseShaderStage(stage_key);

                if (stage_enum != rhi::ShaderStage::Count)
                {
                    ShaderEntryPoint entry =
                    {
                        .stage = stage_enum,
                        .name = entryPoint
                    };

                    arch_res->techniques[pass_name][stage_enum] = entry;
                }
                else
                {
                    PHX_CORE_WARN("Unknown shader stage '{0}' in technique '{1}'", stage_key, pass_name);
                }
            }
        }
    }

    if (YAML::Node rs = root["render_state"]) 
    {
        if (rs["rasterization"]) 
        {
            arch_res->render_state.cull_mode = ParseCullMode(rs["rasterization"]["cull_mode"].as<std::string>());
        }

        if (rs["depth"]) 
        {
            SetIfDefined(rs["depth"], "test", arch_res->render_state.depth_test);
            SetIfDefined(rs["depth"], "write", arch_res->render_state.depth_write);

            std::string cmp;
            SetIfDefined(rs["depth"], "compare_op", cmp);
            arch_res->render_state.depth_compare = ParseCompareOp(cmp);
        }
    }

    if (root["properties"]) 
    {
        uint32_t current_offset = 0;

        for (const auto& it : root["properties"]) 
        {
            std::string key = it.first.as<std::string>();
            YAML::Node val = it.second;

            MaterialVariable& variable;
            layout.offset = current_offset;

            // Type Detection
            if (val.IsScalar()) {
                // Could be float or string (Texture path)
                // We check if it parses as a float successfully
                try {
                    float f = val.as<float>();
                    layout.type = MaterialPropertyType::Float;
                    layout.size = sizeof(float);
                    layout.default_value = ManifestMaterialValue(f);
                }
                catch (...) {
                    // Fallback: It's a string (Texture)
                    layout.type = MaterialPropertyType::Texture;
                    layout.size = sizeof(uint32_t); // Bindless index
                    layout.default_value = ManifestMaterialValue(val.as<std::string>());
                }
            }
            else if (val.IsSequence()) {
                // Vector (Float4, Float3, etc)
                if (val.size() == 4) {
                    layout.type = MaterialPropertyType::Float4;
                    layout.size = sizeof(hlslpp::interop::float4);
                    hlslpp::interop::float4 v(
                        val[0].as<float>(), val[1].as<float>(),
                        val[2].as<float>(), val[3].as<float>()
                    );
                    layout.default_value = ManifestMaterialValue(v);
                }
                // Add checks for size() == 2 or 3 here...
            }

            out_res->property_layout[key] = layout;
            current_offset += layout.size;
        }
        out_res->total_data_size = current_offset;
    }

    // 5. Pack Default Buffer (Same logic as JSON)
    out_res->default_instance_buffer.Allocate(out_res->total_data_size);
    uint8_t* buffer_raw = (uint8_t*)out_res->default_instance_buffer.Data();

    // ... (Memcpy loop is identical to previous response) ...
#endif
    return true;
}