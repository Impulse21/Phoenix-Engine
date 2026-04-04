#include "PhxWorld_pch.h"

#include <PhxWorld/Compiler/GltfPrefabCooker.h>
#include <PhxWorld/Compiler/PrefabManifestSerialization.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Math.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/BinaryBuilder.h>
#include <PhxCore/Platform/Platform.h>

#include <PhxResource/ResourceTypeTraits.h>
#include <PhxResource/IO/StreamingDefintions.h>

#include <PhxRenderer/Shaders/ShaderInterop.h>
#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxRenderer/TextureResourceHandler.h>
#include <PhxRenderer/Compiler/IntermediateMeshExporter.h>
#include <PhxRenderer/Compiler/IntermediateTextureExporter.h>

#include <fstream>
#include <cgltf.h>

using namespace phx;
using namespace phx::renderer;
using namespace phx::resource::compiler;
using namespace hlslpp;

constexpr bool export_dds_for_testing = false;

namespace phx::resource::compilerCookedPathBuilder
{
    std::string ForPrefab(const std::string& source_path)
    {
        std::string dir = GetDirectory(source_path);
        std::string filename = GetFileNameWithoutExt(source_path);
        std::string cache_dir = JoinPaths(dir, ".cache/prefabs/");

        // 3. Assemble the final path with the new extension.
        return JoinPaths(cache_dir, filename + ".phxfab");
    }

    std::string ForMesh(const std::string& source_path, const std::string& sub_asset_name)
    {
        std::string dir = GetDirectory(source_path);
        std::string source_filename = GetFileNameWithoutExt(source_path);
        std::string cache_dir = JoinPaths(dir, ".cache/meshes/");

		// TODO: Use the extenion type from resource
		const char* extension = ResourceTraits<renderer::MeshResource>::Extension;
        std::string new_filename = source_filename + "_" + sub_asset_name + extension;

        return JoinPaths(cache_dir, new_filename);
    }

	std::string ForTexture(const std::string& source_path, const std::string& sub_asset_name)
	{
		std::string dir = GetDirectory(source_path);
		std::string source_filename = GetFileNameWithoutExt(source_path);
		std::string cache_dir = JoinPaths(dir, ".cache/textures/");

		const char* extension = ResourceTraits<renderer::TextureResource>::Extension;;
		std::string new_filename = source_filename + "_" + sub_asset_name + extension;

		return JoinPaths(cache_dir, new_filename);
	}

	std::string ForMaterial(const std::string& source_path, const std::string& sub_asset_name)
	{
		std::string dir = GetDirectory(source_path);
		std::string source_filename = GetFileNameWithoutExt(source_path);
		std::string cache_dir = JoinPaths(dir, ".cache/material/");

		const char* extension = ".phxast";// ResourceTraits<renderer::MaterialResource>::Extension;
		std::string new_filename = source_filename + "_" + sub_asset_name + extension;

		return JoinPaths(cache_dir, new_filename);
	}
}

phx::CGltfPrefabCooker::CGltfPrefabCooker(cgltf_data const& gltf_data, AsyncResourceDescriptor const& resource_description, bool force_recook)
	: m_force_recook(force_recook)
	, m_gltf(gltf_data)
	, m_resource_description(resource_description)
	, m_cgltf_file_attributes(phx::Platform::GetFileAttr(resource_description.os_path_or_pak_path).GetValue())
{
}

bool phx::CGltfPrefabCooker::operator()()
{
	CookMeshes(Span<cgltf_mesh>(m_gltf.meshes, m_gltf.meshes_count));
	CookMaterials(Span<cgltf_material>(m_gltf.materials, m_gltf.materials_count), m_textures);
	CookTextures();

	cgltf_scene* scene = m_gltf.scene;
	Span<cgltf_node*> nodes(scene->nodes, scene->nodes_count);
	WalkNodesRec(nodes);

	const std::string virtual_output_path = CookedPathBuilder::ForPrefab(m_resource_description.virtual_path);
	Result<std::string> os_output_path = IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(virtual_output_path);

	if (os_output_path.HasError())
	{
		PHX_CORE_ERROR("Failed to resolve physical path for prefab output '{0}'", virtual_output_path);
		return false;
	}

	if (!DirectoryExists(os_output_path.GetValue()))
	{
		CreateDirectories(os_output_path.GetValue());
	}

	nlohmann::json prefab_json = m_prefab_manifest;

	std::ofstream out(os_output_path.GetValue());
	out << prefab_json.dump(4); // .dump(4) "pretty prints" the JSON with 4-space indents

	return true;
}

void CGltfPrefabCooker::CookMeshes(Span<cgltf_mesh> cgltf_meshes)
{
    const IVirtualFileSystem* vfs = IVirtualFileSystem::Ptr;

    size_t name_mesh_count = 0;
    for (size_t i = 0; i < cgltf_meshes.size(); ++i)
    {
        const cgltf_mesh& gltf_mesh = cgltf_meshes[i];

        // build mesh name
        std::string mesh_name = gltf_mesh.name ? gltf_mesh.name : "Mesh_" + std::to_string(name_mesh_count++);
        std::string cooked_mesh_virtual_path = CookedPathBuilder::ForMesh(m_resource_description.virtual_path, mesh_name);
        phx::Result<AsyncResourceDescriptor> cooked_mesh_file_descriptor = vfs->GetResourceDescriptorForAsync(cooked_mesh_virtual_path);

		m_mesh_registry[&gltf_mesh] = cooked_mesh_virtual_path;

        // Determine if the mesh is stale
        const bool is_stale = IsCookedResourceStale(cooked_mesh_file_descriptor);

        // Nothing to do here - continuing
		if (!is_stale)
		{
			PHX_CORE_INFO("Mesh '{0}' is up to date. Skipping cook.", mesh_name);
			continue;
		}

		PHX_CORE_INFO("Mesh '{0}' is stale or missing. Cooking to '{1}'", mesh_name, cooked_mesh_virtual_path);
		if (!CGltfIntermediateMeshCooker::Cook(m_gltf, gltf_mesh, cooked_mesh_virtual_path))
		{
			PHX_CORE_ERROR("Failed to cook mesh '{0}' to '{1}'", mesh_name, cooked_mesh_virtual_path);
		}
    }
}

void CGltfPrefabCooker::CookMaterials(Span<cgltf_material> cgltf_mtls, std::unordered_map<std::string, TextureCompileDescriptor>& textures)
{
	const IVirtualFileSystem* vfs = IVirtualFileSystem::Ptr;
	size_t name_counter = 0;
	for (size_t i = 0; i < cgltf_mtls.size(); ++i)
	{
		const cgltf_material& gltf_mtl = cgltf_mtls[i];
		std::string name = gltf_mtl.name ? gltf_mtl.name : "Material_" + std::to_string(name_counter++);
		std::string cooked_virtual_path = CookedPathBuilder::ForMaterial(m_resource_description.virtual_path, name);
		phx::Result<AsyncResourceDescriptor> cooked_file_descriptor = vfs->GetResourceDescriptorForAsync(cooked_virtual_path);

		m_mtl_registry[&gltf_mtl] = cooked_virtual_path;
		const bool is_stale = IsCookedResourceStale(cooked_file_descriptor);
		if (!is_stale)
		{
			PHX_CORE_INFO("Material '{0}' is up to date. Skipping cook.", name);
			continue;
		}

		PHX_CORE_INFO("Material '{0}' is stale or missing. Cooking to '{1}'", name, cooked_virtual_path);
		if (!CGltfMaterialManifestCooker::Cook(m_gltf, gltf_mtl, cooked_virtual_path, m_resource_description.virtual_path, textures))
		{
			PHX_CORE_ERROR("Failed to cook mesh '{0}' to '{1}'", name, cooked_virtual_path);
		}
	}

	// Export Textures
}

void phx::CGltfPrefabCooker::CookTextures()
{
	IVirtualFileSystem* vfs = IVirtualFileSystem::Ptr;

	for (auto& [src_path, compiler_descriptor] : m_textures)
	{
		phx::Result<AsyncResourceDescriptor> cooked_file_descriptor = 
			vfs->GetResourceDescriptorForAsync(compiler_descriptor.virtual_output_path);

		const bool is_stale = IsCookedResourceStale(cooked_file_descriptor);

		if (!is_stale)
		{
			PHX_CORE_INFO("Texture'{0}' is up to date. Skipping cook.", compiler_descriptor.virtual_output_path);
			continue;
		}

		PHX_CORE_INFO(
			"Texture '{0}' is stale or missing. Cooking...",
			compiler_descriptor.virtual_output_path);

		phx::CpuTimer cook_timer;

		phx::Result<IntermediateTexture> intermedaite_texture = TextureCompiler::Compile(vfs, compiler_descriptor);
		if (intermedaite_texture.HasError())
		{
			PHX_CORE_ERROR("Failed to cook texture '{0}' to '{1}'", src_path, compiler_descriptor.virtual_output_path);
			continue;
		}

		PHX_CORE_INFO(
			"Texture '{0}' is cooked successfully {1}ms",
			src_path,
			cook_timer.Elapsed().GetMilliseconds());

		phx::Result<std::string> physical_path = 
			IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(compiler_descriptor.virtual_output_path);

		if (!DirectoryExists(physical_path.GetValue()))
		{
			CreateDirectories(physical_path.GetValue());
		}

		std::ofstream out_file(physical_path.GetValue(), std::ios::binary);
		if (out_file.is_open() == false)
		{
			PHX_ERROR("Failed to open output file '{0}' for writing.", physical_path.GetValue());
			continue;
		}

		PHX_CORE_INFO(
			"Texture '{0}' is exporting to {1}",
			src_path,
			compiler_descriptor.virtual_output_path);

		if (!IntermediateTextureExporter::Export(*intermedaite_texture, out_file))
		{
			PHX_CORE_ERROR("Failed to export texture '{0}' to '{1}'", src_path, compiler_descriptor.virtual_output_path);
			continue;
		}
		if (export_dds_for_testing)
		{
			std::string dds_path = physical_path.GetValue() + ".dds";
			std::ofstream dds_out(dds_path, std::ios::binary);
			PHX_WARN("Exporting texture '{0}' also as BC7 DDS for testing purposes to '{1}'", src_path, dds_path);
			if (IntermediateTextureExporter::ExportBC7ToDDS(*intermedaite_texture, dds_out) == false)
			{
				PHX_CORE_ERROR("Failed to export BC7 DDS texture '{0}' to '{1}'", src_path, dds_path);
				continue;
			}
		}
	}
}

void phx::CGltfPrefabCooker::WalkNodesRec(phx::Span<cgltf_node*> gltf_nodes, int parent_index)
{
	for (auto* gltf_node : gltf_nodes)
	{
		const size_t node_index = m_prefab_manifest.nodes.size();
		PrefabManifest::Node& node_manifest = m_prefab_manifest.nodes.emplace_back();

		node_manifest.parent_index = parent_index;

		if (gltf_node->has_matrix)
		{
			hlslpp::float4x4 local_transform;
			static_assert(sizeof(local_transform) == sizeof(gltf_node->matrix));
			hlslpp::load(local_transform, &gltf_node->matrix[0]);

			hlslpp::float3 scale;
			hlslpp::float4 rotation;
			hlslpp::float3 translation;
			math::Decompose(local_transform, scale, rotation, translation);

			hlslpp::store(&node_manifest.scale.x, scale);
			hlslpp::store(&node_manifest.rotation.x, rotation);
			hlslpp::store(&node_manifest.translation.x, translation);
		}
		else
		{
#if false
			float4x4 local_transform = float4x4::identity();
			const float4x4 scale_matrix = gltf_node->has_scale
				? float4x4::scale(gltf_node->scale[0], gltf_node->scale[1], gltf_node->scale[2])
				: float4x4::identity();

			quaternion rot;
			hlslpp::load(rot, static_cast<const float*>(&gltf_node->rotation[0]));

			const float4x4 rotation_matrix = gltf_node->has_rotation
				? float4x4(rot)
				: float4x4(quaternion::identity());

			float4x4 translation_matrix = gltf_node->has_translation
				? float4x4::translation(gltf_node->translation[0], gltf_node->translation[1], gltf_node->translation[2])
				: float4x4::identity();

			local_transform = translation_matrix * rotation_matrix * scale_matrix;
			node_manifest.local_transform = interop::float4x4(local_transform);
#else
			if (gltf_node->has_scale)
			{
				node_manifest.scale.x = gltf_node->scale[0];
				node_manifest.scale.y = gltf_node->scale[1];
				node_manifest.scale.z = gltf_node->scale[2];
			}

			if (gltf_node->has_rotation)
			{
				node_manifest.rotation.x = gltf_node->rotation[0];
				node_manifest.rotation.y = gltf_node->rotation[1];
				node_manifest.rotation.z = gltf_node->rotation[2];
				node_manifest.rotation.w = gltf_node->rotation[3];
			}

			if (gltf_node->has_rotation)
			{
				node_manifest.translation.x = gltf_node->translation[0];
				node_manifest.translation.y = gltf_node->translation[1];
				node_manifest.translation.z = gltf_node->translation[2];
			}
#endif
		}

		node_manifest.node_type = ManifiestNodeTypeIds::Empty;
		if (gltf_node->mesh)
		{
			// retrieve the mesh.
			auto itr = m_mesh_registry.find(gltf_node->mesh);
			if (itr == m_mesh_registry.end())
			{
				continue;
			}

			node_manifest.node_type = ManifiestNodeTypeIds::Mesh;
			node_manifest.mesh_instance_data = { {} };
			node_manifest.mesh_instance_data->mesh_path = itr->second;

			node_manifest.mesh_instance_data->material_paths.reserve(gltf_node->mesh->primitives_count);
			for (size_t i = 0; i < gltf_node->mesh->primitives_count; ++i)
			{
				cgltf_primitive& prim = gltf_node->mesh->primitives[i];

				auto itr = m_mtl_registry.find(prim.material);
				if (itr == m_mtl_registry.end())
				{
					continue;
				}

				node_manifest.mesh_instance_data->material_paths.push_back(itr->second);
			}
		}

		if (gltf_node->children_count > 0)
		{
			Span<cgltf_node*> nodes(gltf_node->children, gltf_node->children_count);
			WalkNodesRec(nodes, node_index);
		}
	}
}

bool CGltfPrefabCooker::IsCookedResourceStale(phx::Result<AsyncResourceDescriptor> const& cooked_resource_descriptor) const
{
    if (cooked_resource_descriptor.HasError())
        return true;

	if (m_force_recook)
	{
		PHX_CORE_WARN("Forcing recook of resource '{0}'", cooked_resource_descriptor->virtual_path);
		return true;
	}

    Result<PlatformFileAttributes> cooked_resource_attribute = Platform::GetFileAttr(cooked_resource_descriptor->os_path_or_pak_path);

    if (cooked_resource_attribute.HasError())
        return false;

    return cooked_resource_attribute->last_write_time < m_cgltf_file_attributes.last_write_time;
}

phx::CGltfIntermediateMeshCooker::CGltfIntermediateMeshCooker(cgltf_data const& gltf_data, cgltf_mesh const& gltf_mesh, std::string const& virtual_path)
	: m_gltf(gltf_data)
	, m_gltf_mesh(gltf_mesh)
	, m_virtual_path(virtual_path)
{
}

bool CGltfIntermediateMeshCooker::operator()()
{
	math::BoundingSphere sphere_os;
	math::AxisAlignedBox bbox_os;

	Span<cgltf_primitive> primitives(&m_gltf_mesh.primitives[0], m_gltf_mesh.primitives_count);

	std::vector<compiler::IntermediateSubMesh> sub_meshes;
	sub_meshes.reserve(primitives.Size());

	for (auto& primitive : primitives)
	{
		compiler::IntermediateSubMesh& sub_mesh = sub_meshes.emplace_back();

		InitializeSubMesh(sub_mesh, primitive);
		CalculateBounds(sub_mesh);
	}

	compiler::IntermediateMesh intermediate_mesh = compiler::IntermediateMesh::Create(sub_meshes);

	phx::Result<std::string> physical_path = IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(m_virtual_path);
	if (physical_path.HasError())
	{
		PHX_ERROR("Failed to resolve physical path for virtual path '{0}'", m_virtual_path);
		return false;
	}

	if (!DirectoryExists(physical_path.GetValue()))
	{
		CreateDirectories(physical_path.GetValue());
	}

	std::ofstream out_file(physical_path.GetValue(), std::ios::binary);
	if (out_file.is_open() == false)
	{
		PHX_ERROR("Failed to open output file '{0}' for writing.", physical_path.GetValue());
		return false;
	}

	compiler::IntermediateMeshExporter::Export(intermediate_mesh, out_file);

	return true;
}

phx::CGltfMaterialManifestCooker::CGltfMaterialManifestCooker(
	cgltf_data const& /*gltf_data*/,
	cgltf_material const& /*gltf_material*/,
	std::string const& /*output_mtl_virtual_path*/,
	std::string const& /*texture_root_dir*/,
	std::unordered_map<std::string, TextureCompileDescriptor>& /*out_textures*/)
#if false
	: m_out_textures(out_textures)
	//, m_gltf(gltf_data)
	, m_gltf_mtl(gltf_material)
	, m_output_mtl_virtual_path(output_mtl_virtual_path)
	, m_texture_root_dir(texture_root_dir)
#endif
{
}

bool phx::CGltfMaterialManifestCooker::operator()()
{
	using namespace hlslpp;
	
#if false
	MaterialManifest mtl_manifest;
	auto process_textures = [&](const char* prop_name, const cgltf_texture_view& view, TexConversionFlags flags = (TexConversionFlags)0) {
			if (!view.texture || !view.texture->image || !view.texture->image->uri)
				return;

			std::string source_uri = view.texture->image->uri;
			std::string cooked_path = CookedPathBuilder::ForTexture(
				m_texture_root_dir,
				phx::GetFileNameWithoutExt(source_uri));

			mtl_manifest.properties[prop_name] = cooked_path;
			m_out_textures[source_uri] = {
				.virtual_input_path = phx::GetDirectory(m_texture_root_dir) + "/" + source_uri,
				.virtual_output_path = cooked_path,
				.flags = static_cast<TexConversionFlags>(flags | kQualityBC) 
			};
	};

	if (m_gltf_mtl.has_pbr_metallic_roughness)
	{
		mtl_manifest.archetype_name = "standard";
		mtl_manifest.properties["base_colour_factor"] =
			math::LoadInterop<interop::float4>(&m_gltf_mtl.pbr_metallic_roughness.base_color_factor[0]);

		mtl_manifest.properties["metallic_factor"] = m_gltf_mtl.pbr_metallic_roughness.metallic_factor;
		mtl_manifest.properties["roughness_factor"] = m_gltf_mtl.pbr_metallic_roughness.roughness_factor;

		process_textures("base_color_texture", m_gltf_mtl.pbr_metallic_roughness.base_color_texture, TexConversionFlags::kSRGB);
		process_textures("metallic_roughness_texture", m_gltf_mtl.pbr_metallic_roughness.metallic_roughness_texture);
	}

	mtl_manifest.properties["emissive_factor"] =
		math::LoadInterop<interop::float3>(&m_gltf_mtl.emissive_factor[0]);

	mtl_manifest.properties["normal_texture_scale"] = m_gltf_mtl.normal_texture.scale;
	mtl_manifest.properties["alpha_cutoff"] = m_gltf_mtl.alpha_cutoff;

	process_textures("occlusion_texture", m_gltf_mtl.occlusion_texture);
	process_textures("emissive_texture", m_gltf_mtl.emissive_texture);
	process_textures("normal_texture", m_gltf_mtl.normal_texture);


	Result<std::string> os_output_path = IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(m_output_mtl_virtual_path);
	if (os_output_path.HasError())
	{
		PHX_CORE_ERROR("Failed to resolve physical path for prefab output '{0}'", m_output_mtl_virtual_path);
		return false;
	}

	if (!DirectoryExists(os_output_path.GetValue()))
	{
		CreateDirectories(os_output_path.GetValue());
	}


	// nlohmann::json material_json = mtl_manifest;
	// std::ofstream out(os_output_path.GetValue());
	// out << material_json.dump(4);
#endif
	return true;
}

namespace
{
	template <typename VertexType>
	void CopyAttributeToVector(std::vector<VertexType>& out_vector, const cgltf_accessor* accessor)
	{
		static_assert(sizeof(VertexType) == sizeof(float) * 4);
		const size_t num_components = cgltf_num_components(accessor->type);

		std::vector<float> temp_floats(accessor->count * num_components);
		cgltf_accessor_unpack_floats(accessor, temp_floats.data(), temp_floats.size());

		out_vector.resize(accessor->count);
		for (cgltf_size i = 0; i < accessor->count; ++i)
		{
			const float* source_floats = &temp_floats[i * num_components];
			void* dest_ptr = &out_vector[i];
			memcpy(dest_ptr, source_floats, num_components * sizeof(float));
		}
	}
	
	void CopyIntegerAttributeToVector(std::vector<hlslpp::uint4>& out_vector, const cgltf_accessor* accessor)
	{
		out_vector.resize(accessor->count);

		// Determine how many components each vertex has (e.g., 4 for VEC4).
		size_t num_components = cgltf_num_components(accessor->type);

		for (cgltf_size i = 0; i < accessor->count; ++i)
		{
			// Create a temporary array to hold the integer components for one vertex.
			cgltf_uint components[4] = { 0, 0, 0, 0 };

			// cgltf_accessor_read_ui reads all integer components for the i-th
			// element and places them in our temporary array. It correctly
			// handles all source types like ubyte, ushort, etc.
			cgltf_accessor_read_uint(accessor, i, components, num_components);

			// Construct the final vector type from the integer components.
			// This assumes your hlslpp::uint4 (or similar) can be constructed this way.
			out_vector[i] = hlslpp::uint4(
				static_cast<uint32_t>(components[0]),
				static_cast<uint32_t>(components[1]),
				static_cast<uint32_t>(components[2]),
				static_cast<uint32_t>(components[3])
			);
		}
	}
}

void phx::CGltfIntermediateMeshCooker::InitializeSubMesh(compiler::IntermediateSubMesh& sub_mesh, cgltf_primitive const& src_prim)
{
	Span<cgltf_attribute> attributes(src_prim.attributes, src_prim.attributes_count);
	for (const auto& attribute : attributes)
	{
		switch (attribute.type)
		{
		case cgltf_attribute_type_position:
			CopyAttributeToVector<hlslpp::float3>(sub_mesh.positions, attribute.data);
			break;

		case cgltf_attribute_type_normal:
			CopyAttributeToVector<hlslpp::float3>(sub_mesh.normals, attribute.data);
			break;

		case cgltf_attribute_type_tangent:
			CopyAttributeToVector<hlslpp::float4>(sub_mesh.tangents, attribute.data);
			break;

		case cgltf_attribute_type_texcoord:
			if (attribute.index == 0)
			{
				CopyAttributeToVector<hlslpp::float2>(sub_mesh.texCoords_0, attribute.data);
			}
			else if (attribute.index == 1)
			{
				CopyAttributeToVector<hlslpp::float2>(sub_mesh.texCoords_1, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported texture coordinate set TEXCOORD_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_color:
			if (attribute.index == 0)
			{
				CopyAttributeToVector<hlslpp::float3>(sub_mesh.colour, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported color set COLOR_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_joints:
			if (attribute.index == 0)
			{
				CopyIntegerAttributeToVector(sub_mesh.joints_0, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported joint set JOINTS_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_weights:
			if (attribute.index == 0)
			{
				CopyAttributeToVector<hlslpp::float4>(sub_mesh.weights_0, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported weight set WEIGHTS_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_invalid:
		case cgltf_attribute_type_custom:
		default:
			// TODO: Convert to a proper error message.
			PHX_CORE_WARN("Unhandled or invalid cgltf attribute type encountered: {0}", static_cast<uint32_t>(attribute.type));
			break;
		}
	}

	// Handle indices separately
	if (src_prim.indices->count != 0) 
	{
		sub_mesh.indices.resize(src_prim.indices->count);
		cgltf_accessor_unpack_indices(src_prim.indices, &sub_mesh.indices[0], sizeof(uint32_t), src_prim.indices->count);
	}

	bool generated_normals = false;
	if (sub_mesh.normals.empty())
	{
		PHX_CORE_INFO("Mesh doens't contain normal data. Generating normals.");
		PHX_CORE_ASSERT(false, "TODO: Generate normals");
		generated_normals = true;
	}

	const bool generate_tangents =
		src_prim.material &&
		src_prim.material->normal_texture.texture &&
		(sub_mesh.tangents.empty() || generated_normals);

	if (generate_tangents || sub_mesh.tangents.empty())
	{
		PHX_CORE_INFO("Generating tangent data.");
		PHX_CORE_WARN("TODO: Generate tangents not implemented");
	}

	if (src_prim.material)
	{
		if (src_prim.material->alpha_mode == cgltf_alpha_mode_blend)
			sub_mesh.pso_flags |= compiler::PSOFlags::kAlphaBlend;

		if (src_prim.material->alpha_mode == cgltf_alpha_mode_mask)
			sub_mesh.pso_flags |= compiler::PSOFlags::kAlphaTest;

		if (src_prim.material->double_sided)
			sub_mesh.pso_flags |= compiler::PSOFlags::kTwoSided;

		sub_mesh.material_index = static_cast<uint32_t>(src_prim.material - m_gltf.materials);
	}
}

void CGltfIntermediateMeshCooker::CalculateBounds(compiler::IntermediateSubMesh& sub_mesh)
{
	PHX_ASSERT(!sub_mesh.positions.empty());
	const std::vector<hlslpp::float3>& position_stream = sub_mesh.positions;

	float3 min_position(std::numeric_limits<float>::max());
	float3 max_position(std::numeric_limits<float>::min());
	for (auto& position : position_stream)
	{
		min_position = hlslpp::min(min_position, position);
		max_position = hlslpp::max(max_position, position);
	}

	float3 sphere_centre_ls = min_position + max_position * 0.5f;
	float1 max_radius_ls_sq;

	for (auto& position : position_stream)
	{
		float1 length_sqrt_ls = hlslpp::length(sphere_centre_ls - position);
		max_radius_ls_sq = hlslpp::max(max_radius_ls_sq, length_sqrt_ls);

		sub_mesh.bbox_ls.AddPoint(position);
	}

	sub_mesh.bounds_ls = math::BoundingSphere(sphere_centre_ls, hlslpp::sqrt(max_radius_ls_sq));
}