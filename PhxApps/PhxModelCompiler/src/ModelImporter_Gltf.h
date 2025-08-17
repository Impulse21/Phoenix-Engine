#pragma once

#include <hlsl++.h>

#include <PhxCore/Span.h>

#include "IModelImporter.h"
#include "ModelBuildData.h"

struct cgltf_data;
struct cgltf_node;
struct cgltf_mesh;

struct Primitive;

class GltfModelImporter : public IModelImporter
{
public:
	phx::Result<ModelData> Import(std::string const& file, ImportOptions const& import_options) override;

private:
	bool ImportMaterials(cgltf_data* gltf_data, ModelData& model_data);
	bool ImportMesh(
		std::vector<Mesh*>& mesh_list,
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
		std::vector<Mesh*>& mesh_list,
		std::vector<std::byte>& geometry_buffer);

private:
	ImportOptions m_import_options;
	cgltf_data* m_cgltf_data;
};

