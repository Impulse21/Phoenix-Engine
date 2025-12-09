#include "PhxRenderer/PhxRenderer_pch.h"
#include "MaterialResourceHandler.h"

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

void phx::renderer::MaterialResourceHandler::PrepareRequest(
    StreamingRequest& request,
    GenericHandle handle,
    phx::IIoQueue* /*queue*/,
    AsyncResourceDescriptor const& resource_descriptor) const
{
    Handle<MaterialResource> mat_handle = handle.To<MaterialResource>();

    std::shared_ptr<char[]> dest = std::make_shared<char[]>(resource_descriptor.length_of_resource);
    request = {
        .operations = {
            {
                .source = {
                    .data = resource_descriptor,
                    .size = resource_descriptor.length_of_resource,
                },
                .destination = {
                    .target = CpuResourceDestinationInfo{.handle = dest.get()},
                    .size = resource_descriptor.length_of_resource,
                }
            }
        }
    };

    request.on_complete = [=](StreamingResult const& result) mutable {

        auto* material_resource = ResourceStore<MaterialResource>::GetHot(mat_handle);

        if (result.error_code != ErrorCode::Success)
        {
            PHX_CORE_ERROR("Failed to load '{0}'", resource_descriptor.virtual_path);
            material_resource->state = ResourceState::Error;
            return;
        }
        const char* buffer_pointer = dest.get();
        nlohmann::json j = nlohmann::json::parse(buffer_pointer, buffer_pointer + resource_descriptor.length_of_resource);
        MaterialManifest manifest = j.get<MaterialManifest>();

        PHX_CORE_WARN("Material archetypes are not setup yet.");
        material_resource->archetype = {};

		// if def this for now as I amnot sure how I want to store these in the resource yet.
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
                break;
            default:
                j = nullptr;
                break;
            }
        }
#endif
        material_resource->state = ResourceState::Loaded;
    };
}