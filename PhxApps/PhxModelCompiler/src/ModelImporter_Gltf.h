#pragma once

#include <hlsl++.h>

#include <PhxCore/Span.h>

#include "IModelImporter.h"
#include "ModelBuildData.h"

struct cgltf_data;
struct cgltf_node;

class GltfModelImporter : public IModelImporter
{
public:
	phx::Result<ModelData> Import(std::string const& file) override;

private:
	bool ImportMaterials(cgltf_data* gltf_data, ModelData& model_data);
	bool ImportMeshes(cgltf_data* gltf_data, ModelData& model_data);
	uint32_t WalkGraph(
		cgltf_data* gltf_data,
		phx::math::BoundingSphere bounding_sphere,
		phx::math::AxisAlignedBox bounding_box,
		std::vector<Mesh*> mesh_list,
		std::vector<std::byte> geometry_buffer,
		phx::Span<cgltf_node> siblings,
		hlslpp::float4x4 parent_xform);
};

