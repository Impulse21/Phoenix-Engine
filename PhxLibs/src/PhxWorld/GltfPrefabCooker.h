#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/Platform/PlatformWrapper.h>

#include <PhxResource/Compiler/IntermediateMesh.h>

#include <hlsl++.h>


struct cgltf_data;
struct cgltf_primitive;
struct cgltf_mesh;
struct cgltf_attribute;

namespace phx
{
    struct AsyncResourceDescriptor;

    namespace CookedPathBuilder
    {
        std::string ForPrefab(const std::string& source_path);
        std::string ForMesh(const std::string& source_path, const std::string& sub_asset_name);
    }

    class CGltfPrefabCooker
    {
    public:
        static bool Cook(cgltf_data const& gltf_data, AsyncResourceDescriptor const& resource_descriptor)
        {
            CGltfPrefabCooker cook(gltf_data, resource_descriptor);
            return cook();
        }

    protected:
        CGltfPrefabCooker(cgltf_data const& gltf_data, AsyncResourceDescriptor const& resource_description);

        bool operator()();

        void CookMeshes(Span<cgltf_mesh> cgltf_meshes);

    private:
        bool IsCookedResourceStale(phx::Result<AsyncResourceDescriptor> const& cooked_resource_descriptor) const;

    private:
        const cgltf_data& m_gltf;
        const AsyncResourceDescriptor& m_resource_description;
        platform::PlatformFileAttributes m_cgltf_file_attributes;
        std::unordered_map<const cgltf_mesh*, std::string> m_cooked_files_registery;

    };

    class CGltfMeshCooker
    {
    public:
        static bool Cook(cgltf_data const& gltf_data, cgltf_mesh const& gltf_mesh)
        {
            CGltfMeshCooker cook(gltf_data, gltf_mesh);
            return cook();
        }

    protected:
        CGltfMeshCooker(cgltf_data const& gltf_data, cgltf_mesh const& gltf_mesh);
        bool operator()();

    protected:
        void InitializeSubMesh(compiler::IntermediateSubMesh& sub_mesh, cgltf_primitive const& src_prim);
        void CalculateBounds(compiler::IntermediateSubMesh& sub_mesh);
    
    private:
        const cgltf_data& m_gltf;
        const cgltf_mesh& m_gltf_mesh;
    };
}

