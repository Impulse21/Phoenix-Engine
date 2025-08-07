#include "ModelImporter_Gltf.h"

#include <PhxCore/Base.h>
#include <PhxCore/Log.h>
#include <PhxCore/SystemTime.h>

#include <memory>
#include <map>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phx;
using namespace phx::math;
using namespace hlslpp;

namespace
{
	struct CgltfContext
	{
	};

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
}

phx::Result<ModelData> GltfModelImporter::Import(std::string const& file)
{
	CgltfContext ctx = {};
	cgltf_options options = { };

	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &ctx;

	cgltf_data* gltf_data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, file.c_str(), &gltf_data);

	if (result != cgltf_result_success)
	{
		PHX_ERROR("Couldn't parse glTF file '{0}'", file);
		return phx::make_unexpected(~0ull);
	}

	result = cgltf_load_buffers(&options, gltf_data, file.c_str());
	if (result != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF Binary data '{0}'", result);
		return phx::make_unexpected(~0ull);
	}

	phx::CpuTimer timer;
	// Walk the Scene 
	ModelData model_data = {};

	ImportMaterials(gltf_data, model_data);

	// Walk scene graph and import meshes and build Object Transforms
	
	// TODO: Support scene selections
	cgltf_scene* scene = gltf_data->scene;
	if (!scene)
	{
		PHX_ERROR("Attempting to import a model with no scene definition '{0}'", file.c_str());
	}

	model_data.bounding_sphere = {};
	model_data.bounding_box = {};

	uint32_t num_nodes = WalkGraph(
		gltf_data,
		phx::Span(*scene->nodes, scene->nodes_count),
		hlslpp::float4x4::identity(),
		model_data.bounding_sphere,
		model_data.bounding_box,
		model_data.meshes,
		model_data.geometry_data);

	return model_data;
}

bool GltfModelImporter::ImportMaterials(cgltf_data* gltf_data, ModelData& model_data)
{
	model_data.texture_names.resize(gltf_data->images_count);
	for (cgltf_size i = 0; i < gltf_data->images_count; i++)
		model_data.texture_names[i] = gltf_data->images[i].name;

	std::map<std::string, uint8_t> texture_options;

	const cgltf_size num_materials = gltf_data->materials_count;
	model_data.material_constants.resize(num_materials);
	model_data.material_textures.resize(num_materials);

	for (cgltf_size i = 0; i < num_materials; i++)
	{
		const cgltf_material& gltf_mtl = gltf_data->materials[i];
		MaterialConstantData& dst_mtl_data = model_data.material_constants[i];

		PHX_ASSERT(gltf_mtl.has_pbr_metallic_roughness);
		dst_mtl_data.base_colour_factor[0]	= gltf_mtl.pbr_metallic_roughness.base_color_factor[0];
		dst_mtl_data.base_colour_factor[1]	= gltf_mtl.pbr_metallic_roughness.base_color_factor[1];
		dst_mtl_data.base_colour_factor[2]	= gltf_mtl.pbr_metallic_roughness.base_color_factor[2];
		dst_mtl_data.base_colour_factor[3]	= gltf_mtl.pbr_metallic_roughness.base_color_factor[3];
		dst_mtl_data.emissive_factor[0]		= gltf_mtl.emissive_factor[0];
		dst_mtl_data.emissive_factor[1]		= gltf_mtl.emissive_factor[1];
		dst_mtl_data.emissive_factor[2]		= gltf_mtl.emissive_factor[2];
		dst_mtl_data.normal_texture_scale	= gltf_mtl.normal_texture.scale;
		dst_mtl_data.metallic_factor		= gltf_mtl.pbr_metallic_roughness.metallic_factor;
		dst_mtl_data.roughness_factor		= gltf_mtl.pbr_metallic_roughness.roughness_factor;
		dst_mtl_data.flags					= 0;

		MaterialTextureData& dst_mtl_textures = model_data.material_textures[i];
		dst_mtl_textures.address_modes = 0;
		
		std::array<cgltf_texture*, kNumTextures> src_texture_map = {
			gltf_mtl.pbr_metallic_roughness.base_color_texture.texture,
			gltf_mtl.pbr_metallic_roughness.metallic_roughness_texture.texture,
			gltf_mtl.occlusion_texture.texture,
			gltf_mtl.emissive_texture.texture,
			gltf_mtl.normal_texture.texture
		};

		for (uint32_t ti = 0; ti < kNumTextures; ++ti)
		{
			dst_mtl_textures.string_idx[ti] = 0xFFFF;

			cgltf_texture* src_tex = src_texture_map[ti];
			if (src_tex != nullptr)
			{
				const ptrdiff_t index = src_tex->image - gltf_data->images;
				dst_mtl_textures.string_idx[ti] = uint16_t(index);

				if (src_tex->sampler != nullptr)
				{
					dst_mtl_textures.address_modes |= src_tex->sampler->wrap_s << (ti * 4);
					dst_mtl_textures.address_modes |= src_tex->sampler->wrap_t << (ti * 4 + 2);
				}
				else
				{
					dst_mtl_textures.address_modes |= 0x5 << (ti * 4);
				}
			}
			else
			{
				dst_mtl_textures.address_modes |= 0x5 << (ti * 4);
			}
		}
		// Set Texture Options
		auto SetTextureOptions = [&](cgltf_texture* texture, uint8_t options) {
			if (texture && texture->image && texture_options.find(texture->image->name) == texture_options.end())
				texture_options[texture->image->name] = options;
		};


		SetTextureOptions(src_texture_map[kBaseColor], TextureOptions(true, gltf_mtl.alpha_mode == cgltf_alpha_mode_blend  || gltf_mtl.alpha_mode == cgltf_alpha_mode_mask));
		SetTextureOptions(src_texture_map[kMetallicRoughness], TextureOptions(false));
		SetTextureOptions(src_texture_map[kOcclusion], TextureOptions(false));
		SetTextureOptions(src_texture_map[kEmissive], TextureOptions(true));
		SetTextureOptions(src_texture_map[kNormal], TextureOptions(false));
	}

	model_data.texture_options.clear();
	for (auto name : model_data.texture_names)
	{
		auto iter = texture_options.find(name);
		if (iter != texture_options.end())
		{
			model_data.texture_options.push_back(iter->second);
			// Can compile Textures here if we want to.
		}
		else
			model_data.texture_options.push_back(0xFF);
	}

	PHX_ASSERT(model_data.texture_options.size() == model_data.texture_names.size());

	return true;
}

bool GltfModelImporter::ImportMeshes(cgltf_data* gltf_data, ModelData& model_data)
{
	return true;
}

bool GltfModelImporter::ImportMesh(
	std::vector<Mesh*>& mesh_list,
	std::vector<std::byte>& geometry_buffer,
	cgltf_mesh* gltf_mesh,
	float4x4 const& local_to_object,
	phx::math::BoundingSphere& sphere_object_space,
	phx::math::AxisAlignedBox& box_object_space)
{
	return false;
}

uint32_t GltfModelImporter::WalkGraph(
	cgltf_data* gltf_data,
	phx::Span<cgltf_node> siblings,
	hlslpp::float4x4 const& parent_xform,
	phx::math::BoundingSphere& model_bounding_sphere,
	phx::math::AxisAlignedBox& model_bounding_box,
	std::vector<Mesh*>& mesh_list,
	std::vector<std::byte>& geometry_buffer)
{
	for (const auto& sibling : siblings)
	{
		// calculate transform
		float4x4 local_transform = float4x4::identity();
		if (sibling.has_matrix)
		{
			static_assert(sizeof(float4x4) == sizeof(sibling.matrix));
			hlslpp::load(local_transform, static_cast<const float*>(&sibling.matrix[0]));
		}
		else
		{
			const float4x4 scale_matrix = sibling.has_scale
				? float4x4::scale(sibling.scale[0], sibling.scale[1], sibling.scale[2])
				: float4x4::identity();

			quaternion rot;
			hlslpp::load(rot, static_cast<const float*>(&sibling.rotation[0]));

			const float4x4 rotation_matrix = sibling.has_rotation
				? float4x4(rot)
				: float4x4(quaternion::identity());

			float4x4 translation_matrix = sibling.has_translation
				? float4x4::translation(sibling.translation[0], sibling.translation[1], sibling.translation[2])
				: float4x4::identity();

			local_transform = translation_matrix * rotation_matrix * scale_matrix;
		}

		const hlslpp::float4x4 object_transform = local_transform * parent_xform;

		if (!sibling.camera && sibling.mesh)
		{
			BoundingSphere sphere_object_space;
			AxisAlignedBox box_object_space;
			ImportMesh(mesh_list, geometry_buffer, sibling.mesh, object_transform, sphere_object_space, box_object_space);
			model_bounding_sphere = model_bounding_sphere.Union(sphere_object_space);
			model_bounding_box.AddBoundingBox(box_object_space);
		}

		if (sibling.children_count > 0)
		{
			WalkGraph(
				gltf_data,
				siblings,
				parent_xform,
				model_bounding_sphere,
				model_bounding_box,
				mesh_list,
				geometry_buffer);
		}
	}

	return 0;
}