#include "phxMeshConvert.h"

#include <array>

#include <Core/phxLog.h>
#include <cgltf/cgltf.h>
#include <mesh-optimizer/meshoptimizer.h>

using namespace phx;
using namespace phx::MeshConverter;

void phx::MeshConverter::OptimizeMesh(
	Primitive& outPrim,
	cgltf_primitive const& inPrim,
	DirectX::XMMATRIX const& localToObject)
{
	// TODO: I AM HERE

    if (inPrim.type != cgltf_primitive_type_triangles ||
        inPrim.attributes_count == 0)
    {
        PHX_INFO("Found unsupported primitive topology");
        return;
    }

#if false

	size_t indexCount = inPrim.ac;
	size_t unindexed_vertex_count = face_count * 3;
	std::vector<unsigned int> remap(index_count); // allocate temporary memory for the remap table
	size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, index_count, &unindexed_vertices[0], unindexed_vertex_count, sizeof(Vertex));

#endif 
	enum kAccessorType
	{
		kPosition = 0,
		kTangents,
		kNormals,
		kUV0,
		kUV1,
		kColour,
		kJoints,
		kWeights,
		kNumAccessors,
	};
	std::array<const cgltf_accessor*, kNumAccessors> cgltfAccessors;

	// Collect intreasted attributes
	for (int iAttr = 0; iAttr < inPrim.attributes_count; iAttr++)
	{
		const auto& cgltfAttribute = inPrim.attributes[iAttr];

		switch (cgltfAttribute.type)
		{
		case cgltf_attribute_type_position:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			cgltfAccessors[kPosition] = cgltfAttribute.data;
			break;

		case cgltf_attribute_type_tangent:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			cgltfAccessors[kTangents] = cgltfAttribute.data;
			break;

		case cgltf_attribute_type_normal:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			cgltfAccessors[kNormals] = cgltfAttribute.data;
			break;

		case cgltf_attribute_type_texcoord:
			if (std::strcmp(cgltfAttribute.name, "TEXCOORD_0") == 0)
			{
				assert(cgltfAttribute.data->type == cgltf_type_vec2);
				assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
				cgltfAccessors[kUV0] = cgltfAttribute.data;
			}
			else if (std::strcmp(cgltfAttribute.name, "TEXCOORD_1") == 0)
			{
				assert(cgltfAttribute.data->type == cgltf_type_vec2);
				assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
				cgltfAccessors[kUV1] = cgltfAttribute.data;
			}
			break;
		case cgltf_attribute_type_color:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			cgltfAccessors[kColour] = cgltfAttribute.data;
			break;
		case cgltf_attribute_type_joints:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			cgltfAccessors[kJoints] = cgltfAttribute.data;
			break;
		case cgltf_attribute_type_weights:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			cgltfAccessors[kWeights] = cgltfAttribute.data;
			break;
		}
	}


	bool is32BitIndices = false;
	if (inPrim.indices == nullptr)
	{
		assert(false && "Need to implement this.");

		uint32_t totalIndices = inPrim.vertex;
		//meshopt_generateVertexRemap
	}
	else
	{
		assert(inPrim.indices->type == cgltf_type_scalar);
		assert(inPrim.indices->component_type == cgltf_component_type_r_8u);

		is32BitIndices = inPrim.indices->component_type == cgltf_component_type_r_32u;

	}
}
