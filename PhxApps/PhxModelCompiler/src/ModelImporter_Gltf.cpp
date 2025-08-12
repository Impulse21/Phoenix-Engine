#include "ModelImporter_Gltf.h"

#include <PhxCore/Base.h>
#include <PhxCore/Log.h>
#include <PhxCore/SystemTime.h>

#include <limits>
#include <memory>
#include <map>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <meshoptimizer/meshoptimizer.h>

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

	enum VertexStreamType
	{
		VertexStream_Position = 0,
		VertexStream_Normal,
		VertexStream_Tangent,
		VertexStream_Texcoord0,
		VertexStream_Texcoord1,
		VertexStream_Colour0,
		VertexStream_Joints0,
		VertexStream_Weights0,
		VertexStream_Count,

	};

	struct VertexStream
	{
		VertexStreamType type;
		size_t vertex_offset;
		size_t element_stride;
		size_t num_elements;
	};

	struct Primitive
	{
		BoundingSphere bounds_ls;	// local space bounds
		BoundingSphere bounds_os;	// object space bounds
		AxisAlignedBox bbox_ls;		// local space AABB
		AxisAlignedBox bbox_os;		// object space AABB
		std::array<std::optional<VertexStream>, VertexStream_Count> vertex_streams;
		std::vector<std::byte> vertex_buffer;
		std::vector<uint32_t> index_buffer;
		std::vector< std::byte> shadow_indices_buffer;

		uint32_t index_count;
		uint32_t vertex_count;

		union
		{
			uint32_t hash;
			struct {
				uint32_t pso_flags : 16;
				uint32_t index_32 : 1;
				uint32_t material_index : 15;
			};
		};
	};


	void PrintStatistics(Primitive const&)
	{
#if false
		meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), kCacheSize, 0, 0);
		meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), sizeof(Vertex));
		meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(mesh.Indices.data(), mesh.Indices.size(), &copy.vertices[0].px, mesh.GetVertexCount(), sizeof(Vertex));

		meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 32, 32, 32);
		meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 14, 64, 128);
		meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 128, 0, 0);

		printf("%-9s: ACMR %f ATVR %f (NV %f AMD %f Intel %f) Overfetch %f Overdraw %f in %.2f msec\n", name, vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr, vfs.overfetch, os.overdraw, (end - start) * 1000);
#endif
	}
	void CreateVertexStream(Primitive& prim, const cgltf_attribute& attribute, VertexStreamType stream_type)
	{
		const cgltf_accessor* accessor = attribute.data;
		PHX_ASSERT(accessor);
		if (!accessor) 
			return;

		size_t num_components = cgltf_num_components(accessor->type);
		PHX_ASSERT(accessor->component_type == cgltf_component_type_r_32f);

		const size_t attribute_size = accessor->count * num_components * sizeof(float);
		const size_t start_offset = prim.vertex_buffer.size();

		prim.vertex_buffer.resize(start_offset + attribute_size);
		cgltf_accessor_unpack_floats(
			accessor,
			(float*)(prim.vertex_buffer.data() + start_offset),
			accessor->count * num_components);

		VertexStream& stream = prim.vertex_streams[stream_type].emplace();

		stream.type = stream_type;
		stream.num_elements = accessor->count;
		stream.vertex_offset = start_offset;
		stream.element_stride = num_components * sizeof(float);
	}

	void BucketAttributes(phx::Span<cgltf_attribute> attributes, Primitive& prim)
	{
		for (uint32_t i = 0; i < attributes.size(); i++)
		{
			const cgltf_attribute& attribute = attributes[i];

			// No need for the local 'stream' pointer anymore
			switch (attribute.type)
			{
			case cgltf_attribute_type_position:
				CreateVertexStream(prim, attribute, VertexStream_Position);
				break;

			case cgltf_attribute_type_normal:
				CreateVertexStream(prim, attribute, VertexStream_Normal);
				break;

			case cgltf_attribute_type_tangent:
				CreateVertexStream(prim, attribute, VertexStream_Tangent);
				break;

			case cgltf_attribute_type_texcoord:
				if (attribute.index == 0)
				{
					CreateVertexStream(prim, attribute, VertexStream_Texcoord0);
				}
				else if (attribute.index == 1)
				{
					CreateVertexStream(prim, attribute, VertexStream_Texcoord1);
				}
				else
				{
					PHX_WARN("Unsupported texture coordinate set TEXCOORD_{0} found.", attribute.index);
				}
				break;

			case cgltf_attribute_type_color:
				if (attribute.index == 0)
				{
					CreateVertexStream(prim, attribute, VertexStream_Colour0);
				}
				else
				{
					PHX_WARN("Unsupported color set COLOR_{0} found.", attribute.index);
				}
				break;

			case cgltf_attribute_type_joints:
				if (attribute.index == 0)
				{
					CreateVertexStream(prim, attribute, VertexStream_Joints0);
				}
				else
				{
					PHX_WARN("Unsupported joint set JOINTS_{0} found.", attribute.index);
				}
				break;

			case cgltf_attribute_type_weights:
				if (attribute.index == 0)
				{
					CreateVertexStream(prim, attribute, VertexStream_Weights0);
				}
				else
				{
					PHX_WARN("Unsupported weight set WEIGHTS_{0} found.", attribute.index);
				}
				break;

			case cgltf_attribute_type_invalid:
			case cgltf_attribute_type_custom:
			default:
				PHX_WARN("Unhandled or invalid cgltf attribute type encountered: {0}", attribute.type);
				break;
			}
		}
	}

	void GenerateMeshIndices(
		Primitive& prim,
		cgltf_primitive const& src_prim,
		phx::SpanMutable<meshopt_Stream> meshopt_vertex_streams,
		size_t total_new_vb_size,
		std::vector<uint32_t>& remap_table)
	{

		PHX_ASSERT(prim.vertex_streams[VertexStream_Position].has_value());
		size_t vertex_count = prim.vertex_streams[VertexStream_Position]->num_elements;
		size_t index_count = 0;
		std::unique_ptr<uint32_t[]> temp_indices = nullptr;

		if (src_prim.indices)
		{
			const cgltf_accessor* accessor = src_prim.indices;
			index_count = accessor->count;
			temp_indices = std::make_unique<uint32_t[]>(index_count);
			for (size_t i = 0; i < index_count; ++i)
			{
				cgltf_accessor_read_uint(accessor, i, &temp_indices[i], 1);
			}
		}
		else
		{
			index_count = vertex_count;
		}

		remap_table.resize(vertex_count);
		size_t unique_vertex_count = meshopt_generateVertexRemapMulti(
			remap_table.data(),
			temp_indices.get(),
			index_count,
			vertex_count,
			meshopt_vertex_streams.data(),
			meshopt_vertex_streams.Size());

		prim.index_buffer.resize(index_count * sizeof(uint32_t));
		meshopt_remapIndexBuffer(
			prim.index_buffer.data(),
			temp_indices.get(),
			index_count,
			remap_table.data());

		std::vector<std::byte> final_vertex_buffer;
		final_vertex_buffer.reserve(total_new_vb_size * unique_vertex_count); // Pre-allocate

		for (auto& vertex_stream : prim.vertex_streams)
		{
			if (!vertex_stream.has_value())
				continue;

			const size_t required_size = vertex_stream->num_elements * vertex_stream->element_stride;
			vertex_stream->vertex_offset = final_vertex_buffer.size();
			final_vertex_buffer.resize(final_vertex_buffer.size() + required_size);

			void* vertex_data = prim.vertex_buffer.data() + vertex_stream->vertex_offset;
			meshopt_remapVertexBuffer(
				final_vertex_buffer.data() + vertex_stream->vertex_offset,
				vertex_data,
				vertex_stream->num_elements,
				vertex_stream->element_stride,
				remap_table.data());
		}

		prim.vertex_buffer = std::move(final_vertex_buffer);
		prim.vertex_count = vertex_count;
		prim.index_count = index_count;
	}

	void CalculatePrimtiveBounds(Primitive& prim, hlslpp::float4x4 const& local_to_object)
	{
		PHX_ASSERT(prim.vertex_streams[VertexStream_Position].has_value());
		const VertexStream& position_stream = prim.vertex_streams[VertexStream_Position].value();
		const size_t vertex_count = position_stream.num_elements;

		PHX_ASSERT(position_stream.element_stride == sizeof(float) * 3);

		float* position_data = reinterpret_cast<float*>(prim.vertex_buffer.data() + position_stream.vertex_offset);
		
		float3 min_position(std::numeric_limits<float>::max());
		float3 max_position(std::numeric_limits<float>::min());
		for (size_t i = 0; i < vertex_count; i++)
		{
			hlslpp::float3 position(position_data[i * 3 + 0], position_data[i * 3 + 1], position_data[i * 3 + 2]);
			hlslpp::min(min_position, position);
			hlslpp::max(max_position, position);
		}

		
		float3 sphere_centre_ls = min_position + max_position * 0.5f;
		float1 max_radius_ls_sq;
		float3 sphere_centre_os = hlslpp::mul(local_to_object, float4(sphere_centre_ls, 0.0f)).xyz;
		float1 max_radius_os_sq = 0;
	}

	void OptimizePrimitive(Primitive& prim, cgltf_primitive const& src_prim)
	{
		BucketAttributes(Span(src_prim.attributes, src_prim.attributes_count), prim);

		std::vector<meshopt_Stream> meshopt_vertex_streams;
		meshopt_vertex_streams.reserve(VertexStream_Count);

		size_t total_new_vb_size = 0;
		for (auto& vertex_stream : prim.vertex_streams)
		{
			if (!vertex_stream.has_value())
				continue;

			meshopt_vertex_streams.emplace_back(meshopt_Stream{

				.data = prim.vertex_buffer.data() + vertex_stream->vertex_offset,
				.size = sizeof(float),
				.stride = vertex_stream->element_stride,
				});

			total_new_vb_size += vertex_stream->element_stride;
		}

		std::vector<uint32_t> remap_table;
		GenerateMeshIndices(prim, src_prim, meshopt_vertex_streams, total_new_vb_size, remap_table);

		PrintStatistics(prim);

		uint32_t* index_buffer = reinterpret_cast<uint32_t*>(prim.index_buffer.data());

		// -- Optimize vertex cache ---
		meshopt_optimizeVertexCache(index_buffer, index_buffer, prim.index_count, prim.vertex_count);

		// -- Vertex optmized overdraw ---
		// Not in demo?

		// -- Vertex fetch optimization ---
		meshopt_optimizeVertexFetchRemap(remap_table.data(), index_buffer, prim.index_count, prim.vertex_count);

		for (auto& vertex_stream : prim.vertex_streams)
		{
			if (!vertex_stream.has_value())
				continue;

			void* stream_data = prim.vertex_buffer.data() + vertex_stream->vertex_offset;
			meshopt_remapVertexBuffer(
				stream_data,
				stream_data,
				prim.vertex_count,
				vertex_stream->element_stride,
				remap_table.data());
		}

		PrintStatistics(prim);

		// todo: strink indices and calculate bounding boxes.
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
		SetTextureOptions(src_texture_map[kNormalMap], TextureOptions(false));
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

	BoundingSphere sphereOS;
	AxisAlignedBox bboxOS;

	// Optimize Mesh
	std::vector<Primitive> primitives(gltf_mesh->primitives_count);
	for (uint32_t i = 0; i < gltf_mesh->primitives_count; i++)
	{
		OptimizePrimitive(primitives[i], gltf_mesh->primitives[i]);
		CalculatePrimtiveBounds(primitives[i], local_to_object);

		sphereOS = sphereOS.Union(primitives[i].bounds_os);
		bboxOS.AddBoundingBox(primitives[i].bbox_os);
	}
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