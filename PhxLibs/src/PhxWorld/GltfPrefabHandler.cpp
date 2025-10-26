#include <PhxWorld/PhxWorld_pch.h>

#include "GltfPrefabHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IStreamingManager.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <string>

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

    class CGltfPrefabCooker
    {
    public:
        static bool Cook(cgltf_data* gltf_data)
        {
            CGltfPrefabCooker cook(gltf_data);
            return cook();
        }

    protected:
        CGltfPrefabCooker(cgltf_data* gltf_data);

        bool operator()();

    private:
        std::unordered_map<cgltf_mesh*, std::string> m_cooked_files_registery;

    };
}

namespace CookedPathBuilder
{
    std::string ForPrefab(const std::string& source_path) 
    {
        std::string dir = GetDirectory(source_path);
        std::string filename = GetFileNameWithoutExt(source_path);

        // 2. Construct the new cache directory.
        std::string cache_dir = JoinPaths(dir, ".cache/prefabs/");

        // 3. Assemble the final path with the new extension.
        return JoinPaths(cache_dir, filename + ".phxfab");
    }

    std::string ForMesh(const std::string& source_path, const std::string& sub_asset_name) 
    {
        std::string dir = GetDirectory(source_path);
        std::string source_filename = GetFileNameWithoutExt(source_path);

        std::string cache_dir = JoinPaths(dir, ".cache/meshes/");

        std::string new_filename = source_filename + "_" + sub_asset_name + ".phxmsh";

        return JoinPaths(cache_dir, new_filename);
    }
}

bool phx::GltfPrefabHandler::IsStale(std::string const& virtual_file_path, IVirtualFileSystem* vfs) const
{
    Result<AsyncResourceDescriptor> gltf_resource_descriptor = vfs->GetResourceDescriptorForAsync(virtual_file_path);
    phx::Result<platform::PlatformFileAttributes> gltf_resource_attr = phx::Platform::Get().GetFileAttr(gltf_resource_descriptor->os_path_or_pak_path);

    // cooked prefab path
    std::string cooked_prefab_path = CookedPathBuilder::ForPrefab(virtual_file_path);
    Result<AsyncResourceDescriptor> prefab_resource_descriptor = vfs->GetResourceDescriptorForAsync(cooked_prefab_path);

    // TODO: Handle pack files
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
                    .target = dest,
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

        CookPrefab(prefab_handle_resource, resource_descriptor, dest.get());
     };

    streaming_manager->Submit(std::move(request));
}

void phx::GltfPrefabHandler::CookPrefab(RefCountPtr<PrefabHandleResource> prefab_handle_resource, AsyncResourceDescriptor const& resource_descriptor, void* file_data)
{
    std::string cooked_prefab_path = CookedPathBuilder::ForPrefab(resource_descriptor.virtual_path);

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

    // Cook Meshes Meshes
    // TODO: check if prefab is static by examining the scenes extra section - if it is collapse the hierachy.
    CookMeshes(Span(gltf_data->meshes, gltf_data->meshes_count), cooked_files_registery);

}

void phx::GltfPrefabHandler::CookMeshes(Span<cgltf_mesh> meshes, std::unordered_map<cgltf_mesh*, std::string> cooked_files_registery)
{
    for (const cgltf_mesh& mesh : meshes)
    {
        CookedPathBuilder
        Result<AsyncResourceDescriptor> gltf_resource_descriptor = vfs->GetResourceDescriptorForAsync(virtual_file_path);
        phx::Result<platform::PlatformFileAttributes> gltf_resource_attr = phx::Platform::Get().GetFileAttr(gltf_resource_descriptor->os_path_or_pak_path);

        // cooked prefab path
        std::string cooked_prefab_path = CookedPathBuilder::ForPrefab(virtual_file_path);
        Result<AsyncResourceDescriptor> prefab_resource_descriptor = vfs->GetResourceDescriptorForAsync(cooked_prefab_path);

        // TODO: Handle pack files
        phx::Result<platform::PlatformFileAttributes> cooked_file_attr = phx::Platform::Get().GetFileAttr(prefab_resource_descriptor->os_path_or_pak_path);

        if (cooked_file_attr && gltf_resource_attr)
        {
            return gltf_resource_attr->last_write_time > cooked_file_attr->last_write_time;
        }
    }
}

bool CGltfPrefabCooker::operator()
{
    return false;
}