#include "phxMeshConvert.h"

#include <array>

#include <Core/phxLog.h>
#include <Core/phxBinaryBuilder.h>
#include <Renderer/phxShaderInterop.h>

#include <cgltf/cgltf.h>

#include <mesh-optimizer/meshoptimizer.h>

using namespace phx;
using namespace phx::MeshConverter;
using namespace phx::renderer;

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

	size_t FindMaxIndex(const cgltf_accessor* accessor)
	{
		const size_t indexCount = accessor->count;
		auto [indexSrc, indexStride] = CgltfBufferAccessor(accessor, 0);
		uint32_t maxIndex = 0;
		switch (accessor->component_type)
		{
		case cgltf_component_type_r_8u:
			if (!indexStride)
			{
				indexStride = sizeof(uint8_t);
			}

			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				maxIndex = (uint32_t)std::max(*indexSrc, (uint8_t)maxIndex);
			}
			break;

		case cgltf_component_type_r_16u:
			if (!indexStride)
			{
				indexStride = sizeof(uint16_t);
			}

			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				maxIndex = (uint32_t)std::max(*(uint16_t*)indexSrc, (uint16_t)maxIndex);
			}
			break;

		case cgltf_component_type_r_32u:
			if (!indexStride)
			{
				indexStride = sizeof(uint32_t);
			}

			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				maxIndex = (uint32_t)std::max(*(uint32_t*)indexSrc, (uint32_t)maxIndex);
			}
			break;

		default:
			assert(false);
		}
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

	bool b32BitIndices = false;
	const size_t vertexCount = inPrim.attributes->data->count;
	outPrim.NumVertices = vertexCount;

	BinaryBuilder indexBufferBuilder;
	if (inPrim.indices)
	{
		const size_t indexCount = inPrim.indices->count;
		outPrim.NumIndices = indexCount;

		// copy the indices
		auto [indexSrc, indexStride] = CgltfBufferAccessor(inPrim.indices, 0);
		const size_t maxIndex = FindMaxIndex(inPrim.indices);

		b32BitIndices = maxIndex > 0xFFFF;
		const size_t bufferCount = b32BitIndices ? indexCount : indexCount / 2;

		const size_t offset = indexBufferBuilder.Reserve<uint32_t>(bufferCount);
		indexBufferBuilder.Commit();

		// TODO: Need to fill in the vertex buffer.
		uint32_t* indexDst = indexBufferBuilder.Place<uint32_t>(offset, bufferCount);
		switch (inPrim.indices->component_type)
		{
		case cgltf_component_type_r_8u:
			if (!indexStride)
			{
				indexStride = sizeof(uint8_t);
			}

			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				*indexDst = *(const uint8_t*)indexSrc;

				indexSrc += indexStride;
				indexDst++;
			}
			break;

		case cgltf_component_type_r_16u:
			if (!indexStride)
			{
				indexStride = sizeof(uint16_t);
			}

			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				*indexDst = *(const uint16_t*)indexSrc;

				indexSrc += indexStride;
				indexDst++;
			}
			break;

		case cgltf_component_type_r_32u:
			if (!indexStride)
			{
				indexStride = sizeof(uint32_t);
			}

			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				*indexDst = *(const uint32_t*)indexSrc;

				indexSrc += indexStride;
				indexDst++;
			}
			break;

		default:
			assert(false);
		}
	}
	else
	{
		const size_t indexCount = vertexCount;
		outPrim.NumIndices = indexCount;
		b32BitIndices = indexCount > 0xFFFF;
		const size_t bufferCount = b32BitIndices ? indexCount : indexCount / 2;
		const size_t offset = indexBufferBuilder.Reserve<uint32_t>(bufferCount);
		indexBufferBuilder.Commit();

		uint32_t* indexDst = indexBufferBuilder.Place<uint32_t>(offset, bufferCount);
		for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
		{
			if (b32BitIndices)
			{
				*indexDst = (uint32_t)iIdx;
			}
			else
			{
				*reinterpret_cast<uint16_t*>(indexDst) = (uint16_t)iIdx;
			}

			indexDst++;
		}
	}

	struct AccessorInfoCache
	{
		const cgltf_accessor* Accessor = nullptr;
		size_t DefaultStride = 0;
		size_t Offset = 0;
	};

	BinaryBuilder vertexBufferBuilder;
	vertexBufferBuilder.Reserve<phx::renderer::VertexStreamsHeader>();
	std::array<AccessorInfoCache, phx::renderer::kNumStreams> accessors;

	// Collect intreasted attributes
	for (int iAttr = 0; iAttr < inPrim.attributes_count; iAttr++)
	{
		const auto& cgltfAttribute = inPrim.attributes[iAttr];

		switch (cgltfAttribute.type)
		{
		case cgltf_attribute_type_position:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 3;
			accessors[kPosition] = {
				.Accessor = cgltfAttribute.data,
				.DefaultStride = dataStride,
				.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
			};
			break;

		case cgltf_attribute_type_tangent:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 4;
			accessors[kTangents] = {
				.Accessor = cgltfAttribute.data,
				.DefaultStride = dataStride,
				.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
			};
			break;

		case cgltf_attribute_type_normal:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 3;
			accessors[kNormals] = {
				.Accessor = cgltfAttribute.data,
				.DefaultStride = dataStride,
				.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
			};
			break;

		case cgltf_attribute_type_texcoord:
			if (std::strcmp(cgltfAttribute.name, "TEXCOORD_0") == 0)
			{
				assert(cgltfAttribute.data->type == cgltf_type_vec2);
				assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
				const size_t dataStride = sizeof(float) * 2;
				accessors[kUV0] = {
					.Accessor = cgltfAttribute.data,
					.DefaultStride = dataStride,
					.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
				};
			}
			else if (std::strcmp(cgltfAttribute.name, "TEXCOORD_1") == 0)
			{
				assert(cgltfAttribute.data->type == cgltf_type_vec2);
				assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
				const size_t dataStride = sizeof(float) * 2;
				accessors[kUV1] = {
					.Accessor = cgltfAttribute.data,
					.DefaultStride = dataStride,
					.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
				};
			}
			break;
		case cgltf_attribute_type_color:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 3;
			accessors[kColour] = {
				.Accessor = cgltfAttribute.data,
				.DefaultStride = dataStride,
				.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
			};
			break;
		case cgltf_attribute_type_joints:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 4;
			accessors[kJoints] = {
				.Accessor = cgltfAttribute.data,
				.DefaultStride = dataStride,
				.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
			};
			break;
		case cgltf_attribute_type_weights:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 4;
			accessors[kWeights] = {
				.Accessor = cgltfAttribute.data,
				.DefaultStride = dataStride,
				.Offset = vertexBufferBuilder.Reserve(dataStride, 16, vertexCount)
			};
			break;
		}
	}

	vertexBufferBuilder.Commit();
	for (size_t i = 0; i < cgltfAccessors.size(); i++)
	{
		auto& [accessor, defaultStride] = cgltfAccessors[i];
		if (accessor)
		{
			continue;
		}

		auto [data, dataStride] = CgltfBufferAccessor(accessor, defaultStride);
		vertexBufferBuilder.Place(dataOffsets[i], data, dataStride * vertexCount);
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
