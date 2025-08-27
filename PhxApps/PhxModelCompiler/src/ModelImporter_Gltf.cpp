#include "ModelImporter_Gltf.h"

#include <PhxCore/Base.h>
#include <PhxCore/Log.h>
#include <PhxCore/Memory/MemoryUtils.h>
#include <PhxCore/BinaryBuilder.h>
#include <PhxCore/SystemTime.h>
#include <PhxRhi/RHICommon.h>
#include <PhxRenderer/shaders/ShaderInterop.h>

#include <limits>
#include <memory>
#include <vector>
#include <memory>
#include <map>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <meshoptimizer/meshoptimizer.h>

using namespace phx;
using namespace phx::math;
using namespace phx::renderer;

using namespace hlslpp;

struct VertexStream
{
	renderer::VertexStreamType type;
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
	std::shared_ptr<std::vector<std::byte>> vertex_buffer;
	std::shared_ptr<std::vector<std::byte>> index_buffer;
	std::shared_ptr<std::vector<std::byte>> shadow_indices_buffer;

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
#endif

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
	void CreateVertexStream(Primitive& prim, const cgltf_attribute& attribute, renderer::VertexStreamType stream_type)
	{
		const cgltf_accessor* accessor = attribute.data;
		PHX_ASSERT(accessor);
		if (!accessor) 
			return;

		size_t num_components = cgltf_num_components(accessor->type);
		PHX_ASSERT(accessor->component_type == cgltf_component_type_r_32f);

		const size_t attribute_size = accessor->count * num_components * sizeof(float);
		const size_t start_offset = prim.vertex_buffer->size();

		prim.vertex_buffer = std::make_shared<std::vector<std::byte>>(start_offset + attribute_size);
		cgltf_accessor_unpack_floats(
			accessor,
			(float*)(prim.vertex_buffer->data() + start_offset),
			accessor->count * num_components);

		VertexStream& stream = prim.vertex_streams[stream_type].emplace();

		stream.type = stream_type;
		stream.num_elements = accessor->count;
		stream.vertex_offset = start_offset;
		stream.element_stride = num_components * sizeof(float);
	}

	void GenerateMeshIndices(
		Primitive& prim,
		cgltf_primitive const& src_prim,
		phx::SpanMutable<meshopt_Stream> meshopt_vertex_streams,
		size_t total_new_vb_size,
		std::vector<uint32_t>& remap_table)
	{

		PHX_ASSERT(prim.vertex_streams[renderer::VertexStream_Position].has_value());
		size_t vertex_count = prim.vertex_streams[renderer::VertexStream_Position]->num_elements;
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


		prim.index_buffer = std::make_shared<std::vector<std::byte>>(index_count * sizeof(uint32_t));
		meshopt_remapIndexBuffer(
			reinterpret_cast<uint32_t*>(prim.index_buffer->data()),
			temp_indices.get(),
			index_count,
			remap_table.data());

		auto stagging_vertex_buffer = std::make_shared<std::vector<std::byte>>();
		stagging_vertex_buffer->reserve(total_new_vb_size * unique_vertex_count); // Pre-allocate

		for (auto& vertex_stream : prim.vertex_streams)
		{
			if (!vertex_stream.has_value())
				continue;

			const size_t required_size = vertex_stream->num_elements * vertex_stream->element_stride;
			vertex_stream->vertex_offset = stagging_vertex_buffer->size();
			stagging_vertex_buffer->resize(stagging_vertex_buffer->size() + required_size);

			void* vertex_data = prim.vertex_buffer->data() + vertex_stream->vertex_offset;
			meshopt_remapVertexBuffer(
				stagging_vertex_buffer->data() + vertex_stream->vertex_offset,
				vertex_data,
				vertex_stream->num_elements,
				vertex_stream->element_stride,
				remap_table.data());
		}

		prim.vertex_buffer = std::move(stagging_vertex_buffer);
		prim.vertex_count = vertex_count;
		prim.index_count = index_count;
	}

	void CalculatePrimtiveBounds(Primitive& prim, hlslpp::float4x4 const& local_to_object)
	{
		PHX_ASSERT(prim.vertex_streams[VertexStream_Position].has_value());
		const VertexStream& position_stream = prim.vertex_streams[VertexStream_Position].value();
		const size_t vertex_count = position_stream.num_elements;

		PHX_ASSERT(position_stream.element_stride == sizeof(float) * 3);

		float* position_data = reinterpret_cast<float*>(prim.vertex_buffer->data() + position_stream.vertex_offset);
		
		float3 min_position(std::numeric_limits<float>::max());
		float3 max_position(std::numeric_limits<float>::min());
		for (size_t i = 0; i < vertex_count; i++)
		{
			hlslpp::float3 position(position_data[i * 3 + 0], position_data[i * 3 + 1], position_data[i * 3 + 2]);
			min_position = hlslpp::min(min_position, position);
			max_position = hlslpp::max(max_position, position);
		}

		
		float3 sphere_centre_ls = min_position + max_position * 0.5f;
		float1 max_radius_ls_sq;

		float3 sphere_centre_os = hlslpp::mul(local_to_object, float4(sphere_centre_ls, 0.0f)).xyz;
		float1 max_radius_os_sq = 0;

		for (size_t i = 0; i < vertex_count; ++i)
		{
			hlslpp::float3 position_ls(position_data[i * 3 + 0], position_data[i * 3 + 1], position_data[i * 3 + 2]);

			float1 length_sqrt_ls = hlslpp::length(sphere_centre_ls - position_ls);
			max_radius_ls_sq = hlslpp::max(max_radius_ls_sq, length_sqrt_ls);

			prim.bbox_ls.AddPoint(position_ls);

			hlslpp::float3 position_os = hlslpp::mul(local_to_object, float4(position_ls, 0.0f)).xyz;

			// -- TODO SWIITCH TO DOT PRODUCT ---
			float1 length_sqrt_os = hlslpp::length(sphere_centre_os - position_os);
			max_radius_os_sq = hlslpp::max(max_radius_os_sq, length_sqrt_os);

			prim.bbox_os.AddPoint(position_os);
		}

		prim.bounds_ls = math::BoundingSphere(sphere_centre_ls, hlslpp::sqrt(max_radius_ls_sq));
		prim.bounds_os = math::BoundingSphere(sphere_centre_os, hlslpp::sqrt(max_radius_os_sq));
	}

}

phx::Result<ModelData> GltfModelImporter::Import(std::string const& file, ImportOptions const& import_options)
{
	m_import_options = import_options;

	CgltfContext ctx = {};
	cgltf_options options = { };

	// options.file.read = &CgltfReadFile;
	// options.file.release = &CgltfReleaseFile;
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
		// TODO: Conver to proper error code.
		PHX_ERROR("Couldn't load glTF Binary data '{0}'", static_cast<uint32_t>(result));
		return phx::make_unexpected(~0ull);
	}

	phx::CpuTimer timer;

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

	WalkGraph(
		gltf_data,
		phx::Span(*scene->nodes, scene->nodes_count),
		hlslpp::float4x4::identity(),
		model_data.bounding_sphere,
		model_data.bounding_box,
		model_data.meshes,
		model_data.geometry_data);

	m_import_options = {};

	cgltf_free(gltf_data);

	return model_data;
}

bool GltfModelImporter::ImportMaterials(cgltf_data* gltf_data, ModelData& model_data)
{
	std::map<std::string, uint8_t> texture_options;

	const cgltf_size num_materials = gltf_data->materials_count;
	model_data.material_dependencies.resize(num_materials);

	for (cgltf_size i = 0; i < num_materials; i++)
	{
		const cgltf_material& gltf_mtl = gltf_data->materials[i];
		MaterialData& dst_mtl_data = model_data.material_dependencies[i];

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
		dst_mtl_data.alpha_blend			= gltf_mtl.alpha_mode == cgltf_alpha_mode_blend;
		dst_mtl_data.alpha_test				= gltf_mtl.alpha_mode == cgltf_alpha_mode_mask;
		
		std::array<cgltf_texture*, kNumTextures> src_texture_map = {
			gltf_mtl.pbr_metallic_roughness.base_color_texture.texture,
			gltf_mtl.pbr_metallic_roughness.metallic_roughness_texture.texture,
			gltf_mtl.occlusion_texture.texture,
			gltf_mtl.emissive_texture.texture,
			gltf_mtl.normal_texture.texture
		};

		auto translate_wrap_mode = [](cgltf_wrap_mode gltf_wrap_mode) {
				switch (gltf_wrap_mode)
				{
				case cgltf_wrap_mode_mirrored_repeat:
				{
					return MaterialTextureData::WrapMode::MirroredRepeat;
				}
				case cgltf_wrap_mode_repeat:
				{
					return MaterialTextureData::WrapMode::Repeat;
				}
				case cgltf_wrap_mode_clamp_to_edge:
				default:
				{
					return MaterialTextureData::WrapMode::ClampToEdge;
				}
				}
			};

		for (uint32_t ti = 0; ti < kNumTextures; ++ti)
		{
			MaterialTextureData& texture_data = dst_mtl_data.texture_data[ti];

			cgltf_texture* src_tex = src_texture_map[ti];
			if (src_tex != nullptr)
			{
				if (src_tex->image->buffer_view != nullptr)
				{
					PHX_ERROR("Unable to support embedded texture at this time. ");
					throw std::runtime_error("Unsupported operation");
				}

				texture_data.name = src_tex->image->uri;
				if (src_tex->sampler == nullptr)
					continue;


				texture_data.wrap_s = translate_wrap_mode(src_tex->sampler->wrap_s);
				texture_data.wrap_t = translate_wrap_mode(src_tex->sampler->wrap_t);
			}
		}

		// TODO: Move to Material Importer
#if false
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
#endif
	}

	// TODO: Move to Material Importer
#if false
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
#endif
	return true;
}

bool GltfModelImporter::ImportMesh(
	std::vector<Mesh*>& mesh_list,
	std::vector<std::byte>& geometry_buffer,
	cgltf_data* gltf_data,
	cgltf_mesh* gltf_mesh,
	float4x4 const& local_to_object,
	phx::math::BoundingSphere& sphere_object_space,
	phx::math::AxisAlignedBox& box_object_space)
{
	BoundingSphere sphere_os;
	AxisAlignedBox bbox_os;

	// Optimize Mesh
	std::vector<Primitive> primitives(gltf_mesh->primitives_count);
	for (uint32_t i = 0; i < gltf_mesh->primitives_count; i++)
	{
		const cgltf_primitive& src_prim = gltf_mesh->primitives[i];
		Primitive& prim = primitives[i];

		InitializePrimitive(prim, src_prim, gltf_data);
		CalculatePrimtiveBounds(prim, local_to_object);

		sphere_os = sphere_os.Union(prim.bounds_os);
		bbox_os.AddBoundingBox(prim.bbox_os);
	}

	sphere_object_space = sphere_os;
	box_object_space = bbox_os;

	std::map<uint32_t, std::vector<Primitive*>> render_meshes;
	for (auto& prim : primitives)
	{
		uint32_t hash = prim.hash;
		render_meshes[hash].push_back(&prim);
	}

	for (auto& [hash, mesh_primitives] : render_meshes)
	{
		BinaryBuilder<OffsetHandle32> geometry_buffer_builder;

		const size_t num_draws = mesh_primitives.size();
		Mesh* mesh = (Mesh*)malloc(sizeof(Mesh) + sizeof(Mesh::Draw) * (num_draws - 1));
		math::BoundingSphere ls_bounding_sphere;

		mesh->bounds[0] = ls_bounding_sphere.centre.x;
		mesh->bounds[1] = ls_bounding_sphere.centre.y;
		mesh->bounds[2] = ls_bounding_sphere.centre.z;
		mesh->bounds[3] = ls_bounding_sphere.radius.x;
		mesh->ib_format = static_cast<uint8_t>(mesh_primitives[0]->index_32 ? phx::RHI::Format::R32_UINT : phx::RHI::Format::R16_UINT);
		// mesh->mesh_cbv = (uint16_t)matrixIdx;
		//mesh->material_cbv = iter.second[0]->materialIdx;
		mesh->pso_flags = mesh_primitives[0]->pso_flags;
		mesh->pso = 0xFFFF;

		// TODO: Skinned
#if false
		if (gltf_mesh.skin >= 0)
		{
			mesh->numJoints = 0xFFFF;
			mesh->startJoint = (uint16_t)srcMesh.skin;
		}
		else
		{
			mesh->numJoints = 0;
			mesh->startJoint = 0xFFFF;
		}
#endif

		// calculate offsets

		std::vector<OffsetHandle32> vb_header_offsets(num_draws);
		std::vector<OffsetHandle32> vb_data_offsets(num_draws);
		std::vector< OffsetHandle32> ib_offsets(num_draws);

		mesh->vb_size = 0;
		mesh->vb_offset = geometry_buffer_builder.GetSize();
		for (size_t i = 0; i < mesh_primitives.size(); i++)
		{
			Primitive* mesh_prim = mesh_primitives[i];

			vb_header_offsets[i] = geometry_buffer_builder.Reserve<renderer::VertexStreamsHeader>();
			vb_data_offsets[i] = geometry_buffer_builder.Reserve(mesh_prim->vertex_buffer->size(), 16u);
			mesh->vb_size += mesh_prim->vertex_buffer->size();

		}

		mesh->ib_size = 0;
		mesh->ib_offset = geometry_buffer_builder.GetSize();
		for (size_t i = 0; i < mesh_primitives.size(); i++)
		{
			Primitive* mesh_prim = mesh_primitives[i];
			ib_offsets[i] = geometry_buffer_builder.Reserve(mesh_prim->index_buffer->size(), 4u);
			mesh->ib_size += mesh_prim->index_buffer->size();
		}

		mesh->num_draws = static_cast<uint16_t>(num_draws);

		for (size_t i = 0; i < mesh_primitives.size(); i++)
		{
			Primitive* mesh_prim = mesh_primitives[i];
			Mesh::Draw& d = mesh->draw[i];
			d.prim_count = mesh_prim->index_count;
			d.base_vertex = vb_header_offsets[i];
			d.start_index = (uint32_t)mesh_prim->index_buffer->size() >> (mesh_prim->index_32 + 1);
		}

		geometry_buffer_builder.Commit();

		for (size_t i = 0; i < mesh_primitives.size(); i++)
		{
			Primitive* mesh_prim = mesh_primitives[i];
			auto* vb_header = geometry_buffer_builder.Place<renderer::VertexStreamsHeader>(vb_header_offsets[i]);

			for (size_t i_attr = 0; i < VertexStream_Count; i_attr++)
			{
				std::optional<VertexStream>& stream = mesh_prim->vertex_streams[i_attr];
				if (!stream.has_value())
					vb_header->Desc[i_attr].Stride4_Offset28 = 0xFFFF;

				vb_header->Desc[i_attr].SetOffset(stream->vertex_offset);
				vb_header->Desc[i_attr].SetStride(stream->element_stride);
			}

			std::byte* vb_buffer_dest = geometry_buffer_builder.Place<std::byte>(vb_data_offsets[i]);
			std::memcpy(vb_buffer_dest, mesh_prim->vertex_buffer->data(), mesh_prim->vertex_buffer->size());

			std::byte* ib_buffer_dest = geometry_buffer_builder.Place<std::byte>(ib_offsets[i]);
			std::memcpy(ib_buffer_dest, mesh_prim->index_buffer->data(), mesh_prim->index_buffer->size());
		}

		std::unique_ptr<IBlob> mesh_geometry_buffer = geometry_buffer_builder.Finalize();
		const size_t offset = geometry_buffer.size();
		geometry_buffer.resize(offset + mesh_geometry_buffer->Size());
		std::memcpy(geometry_buffer.data() + offset, mesh_geometry_buffer->Data(), mesh_geometry_buffer->Size());

		mesh_list.push_back(mesh);
	}

	return true;
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

			ImportMesh(mesh_list, geometry_buffer, gltf_data, sibling.mesh, object_transform, sphere_object_space, box_object_space);

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

void GltfModelImporter::InitializePrimitive(Primitive& prim, cgltf_primitive const& src_prim, cgltf_data* gltf_data)
{
	prim.vertex_buffer = std::make_shared<std::vector<std::byte>>();
	prim.index_buffer = std::make_shared<std::vector<std::byte>>();

	for (uint32_t i = 0; i < src_prim.attributes_count; i++)
	{
		const cgltf_attribute& attribute = src_prim.attributes[i];

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
			// TODO: Convert to a proper error message.
			PHX_WARN("Unhandled or invalid cgltf attribute type encountered: {0}", static_cast<uint32_t>(attribute.type));
			break;
		}
	}

	bool generated_normals = false;
	if (!prim.vertex_streams[VertexStream_Normal].has_value())
	{
		PHX_INFO("Mesh doens't contain normal data. Generating normals.");
		PHX_ASSERT(false, "TODO: Generate normals");
		generated_normals = true;
	}

	if (!prim.vertex_streams[VertexStream_Tangent] || generated_normals)
	{
		PHX_INFO("Generating tangent data.");

		PHX_ASSERT(false, "TODO: Generate tangents");
	}

	if (src_prim.material)
	{
		if (src_prim.material->alpha_mode == cgltf_alpha_mode_blend)
			prim.pso_flags |= PSOFlags::kAlphaBlend;

		if (src_prim.material->alpha_mode == cgltf_alpha_mode_mask)
			prim.pso_flags |= PSOFlags::kAlphaTest;

		if (src_prim.material->double_sided)
			prim.pso_flags |= PSOFlags::kTwoSided;

		prim.material_index = static_cast<uint32_t>(src_prim.material - gltf_data->materials);
	}

	OptimizePrimitive(prim, src_prim);
}

void GltfModelImporter::OptimizePrimitive(Primitive& prim, cgltf_primitive const& src_prim)
{
	// PSO flags
	std::vector<meshopt_Stream> meshopt_vertex_streams;
	meshopt_vertex_streams.reserve(VertexStream_Count);

	size_t total_new_vb_size = 0;
	for (auto& vertex_stream : prim.vertex_streams)
	{
		if (!vertex_stream.has_value())
			continue;

		meshopt_vertex_streams.emplace_back(meshopt_Stream{

			.data = prim.vertex_buffer->data() + vertex_stream->vertex_offset,
			.size = sizeof(float),
			.stride = vertex_stream->element_stride,
			});

		total_new_vb_size += vertex_stream->element_stride;
	}

	std::vector<uint32_t> remap_table;
	GenerateMeshIndices(prim, src_prim, meshopt_vertex_streams, total_new_vb_size, remap_table);

	PrintStatistics(prim);

	uint32_t* index_buffer = reinterpret_cast<uint32_t*>(prim.index_buffer->data());

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

		void* stream_data = prim.vertex_buffer->data() + vertex_stream->vertex_offset;
		meshopt_remapVertexBuffer(
			stream_data,
			stream_data,
			prim.vertex_count,
			vertex_stream->element_stride,
			remap_table.data());
	}

	PrintStatistics(prim);
}
