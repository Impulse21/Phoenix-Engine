#include "ModelImporter_Gltf.h"

#include <PhxCore/Base.h>
#include <PhxCore/Log.h>
#include <PhxCore/SystemTime.h>

#include <memory>
#include <map>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>


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

	cgltf_data* raw_gltf_data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, file.c_str(), &raw_gltf_data);

	if (result != cgltf_result_success)
	{
		PHX_ERROR("Couldn't parse glTF file '{0}'", file);
		return phx::make_unexpected(~0ull);
	}

	result = cgltf_load_buffers(&options, raw_gltf_data, file.c_str());
	if (result != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF Binary data '{0}'", result);
		return phx::make_unexpected(~0ull);
	}

	phx::CpuTimer timer;
	// Walk the Scene 
	ModelData model_data = {};

	ImportMaterials(raw_gltf_data, model_data);
	ImportMeshes(raw_gltf_data, model_data);

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
