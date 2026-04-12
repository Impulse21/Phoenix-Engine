#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/Platform/Platform.h>

#include <PhxResourceCompiler/IntermediateMesh.h>
#include <PhxResourceCompiler/TextureCompiler.h>

#include <PhxWorld/Prefab.def.h>

#include <hlsl++.h>


struct cgltf_data;
struct cgltf_primitive;
struct cgltf_mesh;
struct cgltf_attribute;
struct cgltf_node;
struct cgltf_material;

namespace phx
{
    struct AsyncResourceDescriptor;
}

namespace phx::resource::compiler
{
    namespace CookedPathBuilder
    {
        std::string ForPrefab(const std::string& source_path);
        std::string ForMesh(const std::string& source_path, const std::string& sub_asset_name); 
        std::string ForTexture(const std::string& source_path, const std::string& sub_asset_name);
        std::string ForMaterial(const std::string& source_path, const std::string& sub_asset_name);
    }

    struct PrefabCookDescriptor
    {
        const char* output_filename;
        cgltf_data* gltf_data = nullptr;
        IFileSystem* src_fs = nullptr;
        IFileSystem* output_fs = nullptr;
        PlatformFileAttributes* file_attr = nullptr;
        bool force_recook = false;
    }

    class CGltfPrefabCooker
    {
    public:
        static bool Cook(const PrefabCookDescriptor& desc)
        {
            CGltfPrefabCooker cook(desc);
            return cook();
        }

    protected:
        CGltfPrefabCooker(const PrefabCookDescriptor& desc);

        bool operator()();

        void CookMeshes(Span<cgltf_mesh> cgltf_meshes);
        void CookMaterials(Span<cgltf_material> cgltf_mtls, std::unordered_map<std::string, resource::compiler::TextureCompileDescriptor > & textures);
        void CookTextures();
        void WalkNodesRec(phx::Span<cgltf_node*> siblings, int parent_index = -1);
        
        bool IsCookedResourceStale(phx::Result<AsyncResourceDescriptor> const& cooked_resource_descriptor) const;

    private:
        const PrefabCookDescriptor& m_cook_desc;

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
        void InitializeSubMesh(resource::compiler::IntermediateSubMesh& sub_mesh, cgltf_primitive const& src_prim);
        void CalculateBounds(resource::compiler::IntermediateSubMesh& sub_mesh);
    
    private:
        const cgltf_data& m_gltf;
        const cgltf_mesh& m_gltf_mesh;
        const std::string& m_virtual_path;
    };

    class CGltfMaterialInstanceDefCooker
    {
    public:
        static bool Cook(
            cgltf_data const& gltf_data,
            cgltf_material const& gltf_material,
            std::string const& output_mtl_virtual_path,
            std::string const& texture_root_dir,
            std::unordered_map<std::string, resource::compiler::TextureCompileDescriptor>& out_textures)
        {
            CGltfMaterialInstanceDefCooker cook(
                gltf_data,
                gltf_material,
                output_mtl_virtual_path,
                texture_root_dir,
                out_textures);

            return cook();
        }

    protected:
        CGltfMaterialInstanceDefCooker(
            cgltf_data const& gltf_data,
            cgltf_material const& gltf_material,
            std::string const& output_mtl_virtual_path,
            std::string const& texture_root_dir,
            std::unordered_map<std::string, resource::compiler::TextureCompileDescriptor>& out_textures);

        bool operator()();

    private:
        std::unordered_map<std::string, resource::compiler::TextureCompileDescriptor>& m_out_textures;
        // const cgltf_data& m_gltf;
        const cgltf_material& m_gltf_mtl;
        const std::string& m_output_mtl_virtual_path;
        const std::string& m_texture_root_dir;
    };
}

