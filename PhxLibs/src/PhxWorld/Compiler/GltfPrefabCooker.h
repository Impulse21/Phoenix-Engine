#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/Platform/PlatformWrapper.h>

#include <PhxRenderer/Compiler/IntermediateMesh.h>

#include <PhxWorld/PrefabResource.h>

#include <hlsl++.h>


struct cgltf_data;
struct cgltf_primitive;
struct cgltf_mesh;
struct cgltf_attribute;
struct cgltf_node;

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
        void CookMaterials(Span<cgltf_material> cgltf_mtls);
        void WalkNodesRec(phx::Span<cgltf_node*> siblings, int parent_index = -1);
        
        bool IsCookedResourceStale(phx::Result<AsyncResourceDescriptor> const& cooked_resource_descriptor) const;

    private:
		const bool m_force_recook;
        const cgltf_data& m_gltf;
        const AsyncResourceDescriptor& m_resource_description;
        PrefabManifest m_prefab_manifest = {};
        platform::PlatformFileAttributes m_cgltf_file_attributes;
        std::unordered_map<const cgltf_mesh*, std::string> m_mesh_registry;
        std::unordered_map<const cgltf_material*, std::string> m_mtl_registry;

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


    enum TexConversionFlags : uint8_t
    {
        kSRGB = BIT(0),             // Texture contains sRGB colors
        kPreserveAlpha = BIT(1),    // Keep four channels
        kNormalMap = BIT(2),        // Texture contains normals
        kBumpToNormal = BIT(3),     // Generate a normal map from a bump map
        kDefaultBC = BIT(4),        // Apply standard block compression (BC1-5)
        kQualityBC = BIT(5),        // Apply quality block compression (BC6H/7)
        kFlipVertical = BIT(6),
    };

    inline TexConversionFlags TextureOptions(bool sRGB, bool has_alpha = false, bool invert_y = false)
    {
        // 2. Accumulate inside a raw integer type first
        uint8_t flags = 0;

        if (sRGB)       flags |= kSRGB;
        if (has_alpha)  flags |= kPreserveAlpha;
        if (invert_y)   flags |= kFlipVertical;

        // 3. Explicitly cast back to the Enum
        return static_cast<TexConversionFlags>(flags);
    }

    class CGltfMaterialResourceCooker
    {
    public:
        static bool Cook(
            cgltf_data const& gltf_data,
            cgltf_material const& gltf_material,
            std::string const& virtual_path,
            std::unordered_map<std::string, TexConversionFlags>& out_textures)
        {
            CGltfMaterialResourceCooker cook(gltf_data, gltf_material, virtual_path, out_textures);
            return cook();
        }

    protected:
        CGltfMaterialResourceCooker(
            cgltf_data const& gltf_data,
            cgltf_material const& gltf_material,
            std::string const& virtual_path,
            std::unordered_map<std::string, TexConversionFlags>& out_textures);

        bool operator()();

    private:
        std::unordered_map<std::string, TexConversionFlags>& m_out_textures;
        const cgltf_data& m_gltf;
        const cgltf_material& m_gltf_mtl;
        const std::string& m_virtual_path;
    };
}

