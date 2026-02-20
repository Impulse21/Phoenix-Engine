#include "PhxWorld_pch.h"

#include <PhxWorld/GltfPrefabHandler.h>

#include <PhxWorld/PrefabResource.h>
#include <PhxWorld/Compiler/PrefabManifestSerialization.h>
#include <PhxWorld/Compiler/GltfPrefabCooker.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxResource/ResourceManager.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxRenderer/MeshResource.h>
#include <PhxRenderer/MaterialResource.h>

#include <PhxResource/IO/StreamingDefintions.h>
#include <PhxResource/IO/IoQueue.h>
#include <PhxCore/JobSystem.h>

#include <nlohmann/json.hpp>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <string>
#include <fstream>

using namespace phx;

namespace
{
#if false
    cgltf_result CgltfReadFile(const cgltf_memory_options*, const cgltf_file_options* /*file_options*/, const char* path, cgltf_size* size, void** Data)
    {
		phx::Result<phx::platform::PlatformFileAttributes> file_attr = phx::Platform::Get().GetFileAttr(path);
        if (!file_attr)
        {
            return cgltf_result_file_not_found;
        }

		phx::Result<phx::platform::PlatformFileHandle> file_handle = phx::Platform::Get().OpenFile(path, "rb");

        std::unique_ptr<phx::IBlob> dataBlob = phx::IVirtualFileSystem::Ptr->ReadFileSynchronous(path).ValueOr(nullptr);
        if (!file_handle)
        {
            return cgltf_result_file_not_found;
        }

		std::unique_ptr<char[]> file_data = std::make_unique<char[]>(file_attr->size);
        size_t size_read = phx::Platform::Get().ReadFile(*file_handle, file_data.get(), file_attr->size);

		phx::Platform::Get().CloseFile(*file_handle);
        if (size)
        {
            *size = size_read;
        }

        if (Data)
        {
            *Data = (void*)file_data.release();
        }

        return cgltf_result_success;
    }

    void CgltfReleaseFile(
        const struct cgltf_memory_options*,
        const struct cgltf_file_options*,
        void* data)
    {
        if (data)
            delete[] static_cast<char*>(data);

        data = nullptr;
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


namespace
{
    enum InternalState
    {
        State_Init              = ResourceState::Loading,
        State_Wait_GLTF         = ResourceState::Loading + 1,
        State_Parse_GLTF        = ResourceState::Loading + 2,
        State_Cook_GLTF         = ResourceState::Loading + 3,
        State_Cooking           = ResourceState::Loading + 4,
        State_Load_Prefab       = ResourceState::Loading + 5,
        State_Wait_Prefab       = ResourceState::Loading + 6,
        State_Parse_Prefab      = ResourceState::Loading + 7,
        State_Prefab_finalize   = ResourceState::Loading + 8, 
        State_Check_Dependencies = ResourceState::Waiting_dependencies
    };
}

bool phx::GltfPrefabLoader::IsStale(AsyncResourceDescriptor const& gltf_resource_descriptor, IVirtualFileSystem* vfs) const
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


LoaderStepResult GltfPrefabLoader::Step(LoadContext& ctx) const
{
    RefCountPtr<PrefabResource> prefab_handle = ctx.handle.As<PrefabResource>();
    auto state = ctx.GetInternalState<InternalState>();

    switch (state)
    {
    case State_Init:
    {
        if (!IsStale(ctx.resource_descriptor, IVirtualFileSystem::Ptr))
        {
            ctx.state_index = State_Load_Prefab;
            return LoaderStepResult::Continue;
        }

        ctx.file_buffer = MemoryBuffer(ctx.resource_descriptor.length_of_resource);
        StreamingRequest request = {
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
        ctx.state_index = State_Wait_GLTF;
        return LoaderStepResult::Continue;
    }
    case State_Wait_GLTF:
    {
        auto io_queue = IIoQueue::Ptr;
        if (!io_queue->IsComplete(ctx.io_ticket))
        {
            return LoaderStepResult::Yield;
        }
        auto result = io_queue->GetResult(ctx.io_ticket);
        if (result.error_code != ErrorCode::Success)
        {
            PHX_CORE_ERROR("Failed to load glTF Prefab source file.");
            return LoaderStepResult::Error;
        }

        ctx.state_index = State_Parse_GLTF;
        return LoaderStepResult::Continue;
    }
    case State_Parse_GLTF:
    {
        ctx.job_sync.Add();
        phx::JobSystem::SubmitJob([prefab_handle, ctx = &ctx](const phx::JobContext&) {
            CookPrefab(prefab_handle, ctx->resource_descriptor, ctx->file_buffer.Data());
            ctx->job_sync.Signal();
        }, phx::JobSystem::Priority::Low);

        ctx.state_index = State_Cooking;
        return LoaderStepResult::Continue;
    }
    case State_Cooking:
    {
        if (ctx.job_sync.IsNotCleared())
        {
            return LoaderStepResult::Yield;
        }

        ctx.state_index = State_Load_Prefab;
        return LoaderStepResult::Continue;
    }
    case State_Load_Prefab:
    {
        std::string cooked_resource_virtual_path = CookedPathBuilder::ForPrefab(ctx.resource_descriptor.virtual_path);
        static_assert(sizeof(phx::AsyncResourceDescriptor) == LoadContext::kScratchSize);

        auto cooked_resource_descriptor = ctx.GetScratch<AsyncResourceDescriptor>();
        *cooked_resource_descriptor =
            IVirtualFileSystem::Ptr->GetResourceDescriptorForAsync(cooked_resource_virtual_path).GetValue();

        if (ctx.file_buffer.Size() < cooked_resource_descriptor->length_of_resource)
        {
            ctx.file_buffer = MemoryBuffer(cooked_resource_descriptor->length_of_resource);
		}
        else
        {
            std::memset(ctx.file_buffer.Data(), 0, ctx.file_buffer.Size());
        }

        StreamingRequest request = {
          .operations = {
              {
                  .source = {
                      .data = *cooked_resource_descriptor,
                      .size = cooked_resource_descriptor->length_of_resource,
                  },
                  .destination = {
                      .target = CpuDestination{.address = ctx.file_buffer.Data() },
                      .size = cooked_resource_descriptor->length_of_resource,
                  }
              }
          }
        };

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(request));
        ctx.state_index = State_Wait_Prefab;
		return LoaderStepResult::Continue;
    }
    case State_Wait_Prefab:
    {
        auto io_queue = IIoQueue::Ptr;
        if (!io_queue->IsComplete(ctx.io_ticket))
        {
            return LoaderStepResult::Yield;
        }
        auto result = io_queue->GetResult(ctx.io_ticket);
        if (result.error_code != ErrorCode::Success)
        {
            PHX_CORE_ERROR("Failed to load cooked Prefab resource.");
            return LoaderStepResult::Error;
        }
        
		ctx.state_index = State_Parse_Prefab;
        return LoaderStepResult::Continue;
	}
    case State_Parse_Prefab:
    {
        ctx.job_sync.Add();
        phx::JobSystem::SubmitJob([prefab_handle, ctx = &ctx](const phx::JobContext&) {

            LoadPrefab(*ctx, prefab_handle);
            ctx->job_sync.Signal();

        }, phx::JobSystem::Priority::Low);

        ctx.state_index = State_Prefab_finalize;
        return LoaderStepResult::Continue;
    }
    case State_Prefab_finalize:
    {
        if (ctx.job_sync.IsNotCleared())
        {
            return LoaderStepResult::Yield;
        }

        // Wait on all dependencies?
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
            PHX_CORE_ERROR("Failed to load glTF Prefab dependency.");
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
        throw std::runtime_error("Invalid GLTF prefab loader state.");
    }
    }

    throw std::runtime_error("Invalid GLTF prefab loader state.");
}

void phx::GltfPrefabLoader::CookPrefab(RefCountPtr<PrefabResource> prefab_handle, AsyncResourceDescriptor const& resource_descriptor, void* file_data)
{
	PHX_CORE_INFO("Cooking glTF Prefab '{0}'", resource_descriptor.virtual_path);

    cgltf_options options = {
#if false
        .file = {
            .read = &CgltfReadFile,
            .release = &CgltfReleaseFile,
        }
#endif
    };

    cgltf_data* gltf_data = nullptr;
    cgltf_result result = cgltf_parse(&options, file_data, resource_descriptor.length_of_resource, &gltf_data);
;
    if (result != cgltf_result_success)
    {
        PHX_ERROR("Couldn't parse glTF file '{0}'", resource_descriptor.virtual_path);
        prefab_handle->state = ResourceState::Error;
        return;
    }

    result = cgltf_load_buffers(&options, gltf_data, resource_descriptor.os_path_or_pak_path.c_str());
    if (result != cgltf_result_success)
    {
        // TODO: Conver to proper error code.
        PHX_ERROR("Couldn't load glTF `{0}` Binary data '{1}'"
            , resource_descriptor.virtual_path.c_str()
            , static_cast<uint32_t>(result));

        prefab_handle->state = ResourceState::Error;
        return;
    }

	CGltfPrefabCooker::Cook(*gltf_data, resource_descriptor, g_force_recook);
}

void GltfPrefabLoader::LoadPrefab(LoadContext& ctx, RefCountPtr<PrefabResource> prefab_handle)
{
    auto cooked_resource_descriptor = ctx.GetScratch<AsyncResourceDescriptor>();
    const char* begin = reinterpret_cast<const char*>(ctx.file_buffer.Data());
    const char* end = begin + cooked_resource_descriptor->length_of_resource;

    nlohmann::json j = nlohmann::json::parse(begin, end);
    PrefabManifest manifest = j.get<phx::PrefabManifest>();

    prefab_handle->nodes.reserve(manifest.nodes.size());
    for (const PrefabManifest::Node& manifest_node : manifest.nodes)
    {
        PrefabResource::Node& node = prefab_handle->nodes.emplace_back();
        node.name = manifest_node.name;
        node.parent_index = manifest_node.parent_index;

        hlslpp::load(node.scale, &manifest_node.scale.x);
        hlslpp::load(node.rotation, &manifest_node.rotation.x);
        hlslpp::load(node.translation, &manifest_node.translation.x);
;
        if (manifest_node.node_type == ManifiestNodeTypeIds::Mesh)
        {
            RefCountPtr<renderer::MeshResource> mesh_handle = 
                ResourceManager::Load<renderer::MeshResource>(manifest_node.mesh_instance_data->mesh_path.c_str());

			ctx.dependencies.push_back(mesh_handle);

            MeshNodeData mesh_node_data = {};
            mesh_node_data.mesh = mesh_handle;
            mesh_node_data.materials.reserve(manifest_node.mesh_instance_data->material_paths.size());
            for (auto& mtl_path : manifest_node.mesh_instance_data->material_paths)
            {
                RefCountPtr<renderer::MaterialResource> mtl_handle =
                    ResourceManager::Load<renderer::MaterialResource>(mtl_path.c_str());

                ctx.dependencies.push_back(mtl_handle);

                mesh_node_data.materials.push_back(mtl_handle);
            }

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
            nested_prefab_data.prefab_handle = ResourceManager::Load<PrefabResource>(nested_prefab_path);
            PHX_CORE_ASSERT(false, "Not implemented: Dependency tracking for nested prefabs.");
            node.data = nested_prefab_data;
        }
        else
        {
            node.data = EmptyNodeData{};
        }
    }
}
