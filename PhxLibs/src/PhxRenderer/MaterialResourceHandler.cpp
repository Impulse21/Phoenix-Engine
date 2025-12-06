#include "PhxRenderer/PhxRenderer_pch.h"
#include "MaterialResourceHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

// todo: fix this path
#include <PhxWorld/Compiler/MaterialResourceSerialization.h>

#include <nlohmann/json.hpp>
#include <PhxRhi/PhxRhi.h>

void phx::renderer::MaterialResourceHandler::LoadAsync(
	IIoQueue* io_queue,
	RefCountPtr<Resource> resource,
	AsyncResourceDescriptor const& resource_descriptor) const
{
	RefCountPtr<MaterialResource> material_resource = resource.As<MaterialResource>();
    material_resource->state = Resource::State::Loading;

    std::shared_ptr<char[]> dest = std::make_shared<char[]>(resource_descriptor.length_of_resource);
    StreamingRequest request = {
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
        if (result.error_code != ErrorCode::Success)
        {
            PHX_CORE_ERROR("Failed to load '{0}'", resource_descriptor.virtual_path);
            material_resource->state = Resource::State::Error;
            return;
        }
        const char* buffer_pointer = dest.get();
        nlohmann::json j = nlohmann::json::parse(buffer_pointer, buffer_pointer + resource_descriptor.length_of_resource);
        MaterialManifest manifest = j.get<MaterialManifest>();

        auto* resource_system = phx::ResourceSystem::Ptr;

        PHX_CORE_WARN("Material archetypes are not setup yet.");
        material_resource->archetype = nullptr;

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
                variable.value.texture = resource_system->Get(value.texture_path.c_str());
                break;
            default:
                j = nullptr;
                break;
            }
        }
#endif
        material_resource->state = Resource::State::Loaded;
    };

    io_queue->Submit(std::move(request));
}