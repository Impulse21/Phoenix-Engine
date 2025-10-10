#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/Result.h>

#include "ModelBuildData.h"

struct cgltf_data;
struct cgltf_node;
struct cgltf_mesh;
struct cgltf_primitive;

struct Primitive;

namespace phx::renderer
{
	struct ImportOptions
	{
		bool bake_transforms = false;
	};

	class ModelImporter_Gltf
	{
	public:
		inline static phx::Result<ModelData> Import(ImportOptions const& import_options, SpanMutable<uint8_t> file_data)
		{
			ModelImporter_Gltf import(import_options, file_data);
			return import();
		}

	private:
		ModelImporter_Gltf(ImportOptions const& import_options, SpanMutable<uint8_t> file_data);
		phx::Result<ModelData> operator()();

	private:
		bool ImportMaterials(cgltf_data* gltf_data, ModelData& model_data);
		bool ImportMesh(
			std::vector<std::unique_ptr<::Mesh>>& mesh_list,
			std::vector<std::byte>& geometry_buffer,
			cgltf_data* gltf_data,
			cgltf_mesh* gltf_mesh,
			hlslpp::float4x4 const& local_to_object,
			phx::math::BoundingSphere& sphere_object_space,
			phx::math::AxisAlignedBox& box_object_space);

		uint32_t WalkGraph(
			cgltf_data* gltf_data,
			phx::Span<cgltf_node> siblings,
			hlslpp::float4x4 const& parent_xform,
			phx::math::BoundingSphere& model_bounding_sphere,
			phx::math::AxisAlignedBox& model_bounding_box,
			std::vector<std::unique_ptr<::Mesh>>& mesh_list,
			std::vector<std::byte>& geometry_buffer);


		void InitializePrimitive(Primitive& prim, cgltf_primitive const& src_prim, cgltf_data* gltf_data);
		void OptimizePrimitive(Primitive& prim, cgltf_primitive const& src_prim);
	
	private:
		ImportOptions m_import_options;
		SpanMutable<uint8_t> m_file_data;
	};
}

