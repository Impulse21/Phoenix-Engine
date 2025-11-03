#include <PhxWorld/PhxWorld_pch.h>

#include "GltfPrefabHandler.h"

#include "PrefabResource.h"
#include "Compiler/PrefabManifestSerialization.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IStreamingManager.h>

#include <nlohmann/json.hpp>
#include "Compiler/GltfPrefabCooker.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <string>
#include <fstream>

using namespace phx;

namespace
{
    struct CgltfContext
    {
    };

#if false
    cgltf_result CgltfReadFile(const cgltf_memory_options*, const cgltf_file_options* /*file_options*/, const char* /*path*/, cgltf_size* /*size*/, void** /*Data*/)
    {
#if false
        CgltfContext* context = (CgltfContext*)file_options->user_data;

        std::unique_ptr<phx::IBlob> dataBlob = context->vfs->ReadFileSynchronous(path).ValueOr(nullptr);
        if (!dataBlob)
        {
            return cgltf_result_file_not_found;
        }

        if (size)
        {
            *size = dataBlob->Size();
        }

        if (Data)
        {
            *Data = (void*)dataBlob->Data();  // NOLINT(clang-diagnostic-cast-qual)
        }

        context->Blobs.push_back(std::move(dataBlob));
#endif
        return cgltf_result_success;
    }

    void CgltfReleaseFile(
        const struct cgltf_memory_options*,
        const struct cgltf_file_options*,
        void*)
    {
        // do nothing
    }

    void PrintStatistics(Primitive const&)
    {
        meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), kCacheSize, 0, 0);
        meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), sizeof(Vertex));
        meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(mesh.Indices.data(), mesh.Indices.size(), &copy.vertices[0].px, mesh.GetVertexCount(), sizeof(Vertex));

        meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 32, 32, 32);
        meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 14, 64, 128);
        meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 128, 0, 0);

        printf("%-9s: ACMR %f ATVR %f (NV %f AMD %f Intel %f) Overfetch %f Overdraw %f in %.2f msec\n", name, vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr, vfs.overfetch, os.overdraw, (end - start) * 1000);

    }
#endif

}

bool phx::GltfPrefabHandler::IsStale(AsyncResourceDescriptor const& gltf_resource_descriptor, IVirtualFileSystem* vfs) const
{
    if (g_force_recook)
        return true;

    phx::Result<platform::PlatformFileAttributes> gltf_resource_attr = phx::Platform::Get().GetFileAttr(gltf_resource_descriptor.os_path_or_pak_path);

    // cooked prefab path
    std::string cooked_prefab_path = CookedPathBuilder::ForPrefab(gltf_resource_descriptor.virtual_path);
    Result<AsyncResourceDescriptor> prefab_resource_descriptor = vfs->GetResourceDescriptorForAsync(cooked_prefab_path);

    if (prefab_resource_descriptor.HasError())
    {
        return true;
	}

    phx::Result<platform::PlatformFileAttributes> cooked_file_attr = phx::Platform::Get().GetFileAttr(prefab_resource_descriptor->os_path_or_pak_path);
    if (cooked_file_attr && gltf_resource_attr)
    {
        return gltf_resource_attr->last_write_time > cooked_file_attr->last_write_time;
    }

    return true;
}

void GltfPrefabHandler::LoadAsync(IStreamingManager* streaming_manager, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const
{
    RefCountPtr<PrefabHandleResource> prefab_handle_resource = resource.As<PrefabHandleResource>();

    // TODO: the streaming manager can handle allocating and dealloating stagging buffer data
    std::shared_ptr<char[]> dest = std::make_shared<char[]>(resource_descriptor.length_of_resource);
    StreamingRequest request = {
        .operations = {
            {
                .source = {
                    .data = resource_descriptor,
                    .size = resource_descriptor.length_of_resource,
                },
                .destination = {
                    .target = CpuResourceDestinationInfo{ .handle = dest },
                    .size = resource_descriptor.length_of_resource,
                }
            }
        }
    };

    request.on_complete = [=](StreamingResult const& result) mutable {
        if (result.error_code != ErrorCode::Success)
        {
            PHX_CORE_ERROR("Failed to load '{0}'", resource_descriptor.virtual_path);
            prefab_handle_resource->state = Resource::State::Error;
            return;
        }

        // TODO: Check if resource is stale.
        if (IsStale(resource_descriptor, IVirtualFileSystem::Ptr))
        {
            PHX_CORE_INFO("glTF Prefab '{0}' is stale or missing. Cooking...", resource_descriptor.virtual_path);
            CookPrefab(prefab_handle_resource, resource_descriptor, dest.get());
        }

        std::string cooked_resource_virtual_path = CookedPathBuilder::ForPrefab(resource_descriptor.virtual_path);
        std::string cooked_prefab_physical_path = 
            IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(cooked_resource_virtual_path).GetValue();
        std::ifstream stream(cooked_prefab_physical_path.c_str());

        if (stream.is_open() == false)
        {
            PHX_CORE_ERROR("Failed to open cooked glTF Prefab '{0}'", cooked_resource_virtual_path);
            prefab_handle_resource->state = Resource::State::Error;
            return;
		}

        LoadPrefab(stream, prefab_handle_resource);
     };

    streaming_manager->Submit(std::move(request));
}

void phx::GltfPrefabHandler::CookPrefab(RefCountPtr<PrefabHandleResource> prefab_handle_resource, AsyncResourceDescriptor const& resource_descriptor, void* file_data)
{
	PHX_CORE_INFO("Cooking glTF Prefab '{0}'", resource_descriptor.virtual_path);

    CgltfContext ctx = {};
    cgltf_options options = { };

    // options.file.read = &CgltfReadFile;
    // options.file.release = &CgltfReleaseFile;
    options.file.user_data = &ctx;

    cgltf_data* gltf_data = nullptr;
    cgltf_result result = cgltf_parse(&options, file_data, resource_descriptor.length_of_resource, &gltf_data);
;
    if (result != cgltf_result_success)
    {
        PHX_ERROR("Couldn't parse glTF file '{0}'", resource_descriptor.virtual_path);
        prefab_handle_resource->state = Resource::State::Error;
        return;
    }

    result = cgltf_load_buffers(&options, gltf_data, resource_descriptor.os_path_or_pak_path.c_str());
    if (result != cgltf_result_success)
    {
        // TODO: Conver to proper error code.
        PHX_ERROR("Couldn't load glTF `{0}` Binary data '{1}'"
            , resource_descriptor.virtual_path.c_str()
            , static_cast<uint32_t>(result));

        prefab_handle_resource->state = Resource::State::Error;
        return;
    }

	CGltfPrefabCooker::Cook(*gltf_data, resource_descriptor, g_force_recook);
}

void GltfPrefabHandler::LoadPrefab(std::ifstream& stream, RefCountPtr<PrefabHandleResource> prefab_handle_resource)
{
    nlohmann::json j = nlohmann::json::parse(stream);
    PrefabManifest manifest = j.get<phx::PrefabManifest>();

    prefab_handle_resource->prefab = RefCountPtr<PrefabResource>::Create(new PrefabResource());
    PrefabResource& prefab_resource = *prefab_handle_resource->prefab;

    prefab_resource.nodes.reserve(manifest.nodes.size());
    for (const PrefabManifest::Node& manifest_node : manifest.nodes)
    {
        PrefabResource::Node& node = prefab_resource.nodes.emplace_back();
        node.name = manifest_node.name;
        node.parent_index = manifest_node.parent_index;
		hlslpp::load(node.local_transform, &manifest_node.local_transform.m00);

        if (manifest_node.node_type == ManifiestNodeTypeIds::Mesh)
        {
            MeshNodeData mesh_node_data = {};
            mesh_node_data.mesh = phx::ResourceSystem::Ptr->Get(manifest_node.mesh_instance_data->mesh_path.c_str());
            if (manifest_node.mesh_instance_data->material_path)
                PHX_CORE_ERROR("Material paths are not supported at the moment");

            node.data = mesh_node_data;
        }
        else if (manifest_node.node_type == ManifiestNodeTypeIds::Camera)
        {
            CameraNodeData camera_node_data = {};

            if (manifest_node.camera_data->type == ManifiestCameraTypeIds::Orthographic)
            {
				camera_node_data.type = CameraNodeData::Type::Orthographic;
            }
            else
            {
				camera_node_data.type = CameraNodeData::Type::Perspective;
            }

            camera_node_data.fov_y = manifest_node.camera_data->fov_y;
            camera_node_data.z_near = manifest_node.camera_data->z_near;
            camera_node_data.z_far = manifest_node.camera_data->z_far;

            node.data = camera_node_data;
        }
        else if (manifest_node.node_type == ManifiestNodeTypeIds::Light)
        {
            LightNodeData light_node_data = {};

            if (manifest_node.camera_data->type == ManifiestLightTypeIds::Directional)
            {
                light_node_data.type = LightNodeData::Type::Directional;
            }
            else if (manifest_node.camera_data->type == ManifiestLightTypeIds::Spot)
            {
                light_node_data.type = LightNodeData::Type::Spot;
            }
			else
            {
                light_node_data.type = LightNodeData::Type::Point;
            }

            hlslpp::load(light_node_data.colour, &manifest_node.light_data->colour.x);
            light_node_data.intensity = manifest_node.light_data->intensity;
            node.data = light_node_data;
        }
        else if (manifest_node.node_type == ManifiestNodeTypeIds::Prefab)
        {
            NestedPrefabData nested_prefab_data = {};
			const char* nested_prefab_path = manifest_node.nested_prefab_path->c_str();
            nested_prefab_data.prefabHandle = ResourceSystem::Ptr->Get(nested_prefab_path);
            node.data = nested_prefab_data;
        }
        else
        {
            node.data = EmptyNodeData{};
        }

		prefab_handle_resource->state = Resource::State::Loaded;
    }
}
