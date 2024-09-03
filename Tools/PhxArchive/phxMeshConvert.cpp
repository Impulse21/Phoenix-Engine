#include "phxMeshConvert.h"

#include <array>

#include <Core/phxLog.h>
#include <cgltf/cgltf.h>
#include <mesh-optimizer/meshoptimizer.h>

using namespace phx;
using namespace phx::MeshConverter;

namespace
{
	std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
	{
		// TODO: sparse accessor support
		const cgltf_buffer_view* view = accessor->buffer_view;
		const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
		const size_t stride = view->stride ? view->stride : defaultStride;
		return std::make_pair(Data, stride);
	}
}

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
	std::array<std::unique_ptr<uint8_t[]>, kNumAccessors> vertexData;
	std::memset(cgltfAccessors.data(), 0, cgltfAccessors.size() * sizeof(cgltf_accessor*));

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

	if (cgltfAccessors[kPosition] == nullptr)
	{
		PHX_ERROR("Missing required Position attribute.");
		return;
	}

	const size_t vertexCount = cgltfAccessors[kPosition]->count;
	for (size_t i = 0; i < cgltfAccessors.size(); i++)
	{
		const cgltf_accessor* accessor = cgltfAccessors[i];
		if (accessor)
		{
			return;
		}

		auto [data, dataStride] = CgltfBufferAccessor(accessor, accessor->stride * numComponents);
		vertexData[i] = std::make_unique<uint8_t[]>(vertexCount * dataStride);
		std::memcpy(
			stream.get(),
			data,
			dataStride* accessor->count);
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

		const size_t indexCount = inPrim.indices->count;

		auto [indexSrc, indexStride] = CgltfBufferAccessor(inPrim.indices, 0);
		switch (inPrim.indices->stride)
		{
			case sizeof(uint8_t) :
				for (size_t i = 0; i < indexCount; i += 3)
				{
					outMesh.Indices[indexOffset + i + 0] = vertexOffset + indexSrc[i + indexRemap[0]];
					outMesh.Indices[indexOffset + i + 1] = vertexOffset + indexSrc[i + indexRemap[1]];
					outMesh.Indices[indexOffset + i + 2] = vertexOffset + indexSrc[i + indexRemap[2]];
				}
			break;
			case sizeof(uint16_t) :
				for (size_t i = 0; i < indexCount; i += 3)
				{
					outMesh.Indices[indexOffset + i + 0] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[0]];
					outMesh.Indices[indexOffset + i + 1] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[1]];
					outMesh.Indices[indexOffset + i + 2] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[2]];
				}
			break;
			case sizeof(uint32_t) :
				is32BitIndices = true;
				for (size_t i = 0; i < indexCount; i += 3)
				{
					outMesh.Indices[indexOffset + i + 0] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[0]];
					outMesh.Indices[indexOffset + i + 1] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[1]];
					outMesh.Indices[indexOffset + i + 2] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[2]];
				}
			break;
			default:
				assert(0 && "unsupported index stride!");
	}


	assert(material.index < 0x8000 && "Only 15-bit material indices allowed");
}
