#include "PhxRenderer_pch.h"
#include <PhxRenderer/MaterialArchetypeHandler.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxResource/ResourceManager.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

// todo: fix this path
#include <PhxWorld/Compiler/MaterialResourceSerialization.h>

#include <yaml-cpp/yaml.h>
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

    template <typename T>
    void SetIfDefined(const YAML::Node& node, const std::string& key, T& destination)
    {
        if (YAML::Node val = node[key]) 
        {
            destination = val.as<T>();
        }
    }

    rhi::ShaderStage ParseShaderStage(const std::string& key)
    {
        std::string s = key;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);

        if (s == "vertex" || s == "vs")
            return rhi::ShaderStage::VS;

        if (s == "fragment" || s == "pixel" || s == "ps") 
            return rhi::ShaderStage::PS;

        if (s == "compute" || s == "cs")
            return rhi::ShaderStage::CS;

        if (s == "mesh" || s == "ms")
            return rhi::ShaderStage::MS;

        if (s == "amplification" || s == "task" || s == "as")
            return rhi::ShaderStage::AS;

        if (s == "geometry" || s == "gs")
            return rhi::ShaderStage::GS;

        if (s == "hull" || s == "tesscontrol" || s == "hs")
            return rhi::ShaderStage::HS;

        if (s == "domain" || s == "tesseval" || s == "ds")
            return rhi::ShaderStage::DS;

        if (s == "library" || s == "lib")
            return rhi::ShaderStage::LIB;

        return rhi::ShaderStage::Count;
    }

    rhi::RasterCullMode ParseCullMode(const std::string& s)
    {
        if (s.empty()) 
            return rhi::RasterCullMode::Back;

        switch (s[0])
        {
        case 'B':
        case 'b':
            return rhi::RasterCullMode::Back;

        case 'F':
        case 'f':
            return rhi::RasterCullMode::Front;

        case 'N':
        case 'n':
            return rhi::RasterCullMode::None;

        default:
            return rhi::RasterCullMode::Back;
        }
    }

    rhi::ComparisonFunc ParseCompareOp(const std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);

        static const std::unordered_map<std::string, rhi::ComparisonFunc> lookup = 
        {
            { "never",          rhi::ComparisonFunc::Never },
            { "less",           rhi::ComparisonFunc::Less },
            { "equal",          rhi::ComparisonFunc::Equal },
            { "lessorequal",    rhi::ComparisonFunc::LessOrEqual },
            { "lessequal",      rhi::ComparisonFunc::LessOrEqual },
            { "greater",        rhi::ComparisonFunc::Greater },
            { "notequal",       rhi::ComparisonFunc::NotEqual },
            { "greaterorequal", rhi::ComparisonFunc::GreaterOrEqual },
            { "greaterequal",   rhi::ComparisonFunc::GreaterOrEqual },
            { "always",         rhi::ComparisonFunc::Always }
        };

        // 3. Find and return (Default to LessOrEqual if not found)
        auto it = lookup.find(s);
        if (it != lookup.end())
            return it->second;

        return rhi::ComparisonFunc::LessOrEqual;
    }
}

LoaderStepResult MaterialArchetypeResourceHandler::Step(LoadContext& ctx) const
{
    RefCountPtr<MaterialArchetypeResource> arch_handle = ctx.handle.As<MaterialArchetypeResource>();
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
        phx::JobSystem::SubmitJob([arch_handle, ctx = &ctx](const phx::JobContext&) {
            LoadArchetype(*ctx, arch_handle);
            ctx->job_sync.Signal();
        }, phx::JobSystem::Priority::Low);

        ctx.state_index = State_Wait_For_Parse;
        return LoaderStepResult::Continue;
    }
    case State_Wait_For_Parse:
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

bool phx::renderer::MaterialArchetypeResourceHandler::LoadArchetype(LoadContext& ctx, RefCountPtr<MaterialArchetypeResource> arch_res)
{
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

    return true;
}