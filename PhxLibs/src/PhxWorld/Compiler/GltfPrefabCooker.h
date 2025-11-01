#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/Platform/PlatformWrapper.h>

#include <PhxRenderer/Compiler/IntermediateMesh.h>

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
        static bool Cook(cgltf_data const& gltf_data, AsyncResourceDescriptor const& resource_descriptor, bool force_recook = false)
        {
            CGltfPrefabCooker cook(gltf_data, resource_descriptor, force_recook);
            return cook();
        }

    protected:
        CGltfPrefabCooker(cgltf_data const& gltf_data, AsyncResourceDescriptor const& resource_description, bool force_recook);

        bool operator()();

        void CookMeshes(Span<cgltf_mesh> cgltf_meshes);

    private:
        bool IsCookedResourceStale(phx::Result<AsyncResourceDescriptor> const& cooked_resource_descriptor) const;

    private:
		const bool m_force_recook;
        const cgltf_data& m_gltf;
        const AsyncResourceDescriptor& m_resource_description;
        platform::PlatformFileAttributes m_cgltf_file_attributes;
        std::unordered_map<const cgltf_mesh*, std::string> m_cooked_files_registery;

    };

    class CGltfIntermediateMeshCooker
    {
    public:
        static bool Cook(cgltf_data const& gltf_data, cgltf_mesh const& gltf_mesh, std::string const& virtual_path)
        {
            CGltfIntermediateMeshCooker cook(gltf_data, gltf_mesh, virtual_path);
            return cook();
        }

    protected:
        CGltfIntermediateMeshCooker(cgltf_data const& gltf_data, cgltf_mesh const& gltf_mesh, std::string const& virtual_path);
        bool operator()();

    protected:
        void InitializeSubMesh(renderer::compiler::IntermediateSubMesh& sub_mesh, cgltf_primitive const& src_prim);
        void CalculateBounds(renderer::compiler::IntermediateSubMesh& sub_mesh);
    
    private:
        const cgltf_data& m_gltf;
        const cgltf_mesh& m_gltf_mesh;
        const std::string& m_virtual_path;
    };
}

