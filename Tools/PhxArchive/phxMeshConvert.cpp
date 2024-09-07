#include "phxMeshConvert.h"

#include <array>

#include <Core/phxLog.h>
#include <Core/phxBinaryBuilder.h>
#include <Core/phxMath.h>
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
	size_t indexCount = 0;
	outPrim.NumVertices = vertexCount;

	BinaryBuilder indexBufferBuilder;
	if (inPrim.indices)
	{
		indexCount = inPrim.indices->count;
		outPrim.NumIndices = indexCount;

		// copy the indices
		auto [indexSrc, indexSrcStride] = CgltfBufferAccessor(inPrim.indices, 0);
		const size_t maxIndex = FindMaxIndex(inPrim.indices);

		b32BitIndices = maxIndex > 0xFFFF;
		const size_t destStride = b32BitIndices ? 4 : 2;
		const size_t bufferSize = destStride * indexCount;
		const size_t offset = indexBufferBuilder.Reserve<uint8_t>(bufferSize);
		indexBufferBuilder.Commit();

		// TODO: Need to fill in the vertex buffer.
		uint8_t* indexDst = indexBufferBuilder.Place<uint8_t>(offset, bufferSize);
		switch (inPrim.indices->component_type)
		{
		case cgltf_component_type_r_8u:
			indexSrcStride = sizeof(uint8_t);
			break;
		case cgltf_component_type_r_16u:
			indexSrcStride = sizeof(uint16_t);
			break;
		case cgltf_component_type_r_32u:
			indexSrcStride = sizeof(uint32_t);
			break;
		default:
			assert(false);
		}

		for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
		{
			std::memcpy(
				indexDst,
				indexSrc,
				indexSrcStride);

			indexSrc += indexSrcStride;
			indexDst += destStride;
		}
	}
	else
	{
		indexCount = vertexCount;
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

	outPrim.IndexBuffer = indexBufferBuilder.GetMemory();

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
	for (size_t i = 0; i < accessors.size(); i++)
	{
		const AccessorInfoCache& accessorInfo = accessors[i];
		if (!accessorInfo.Accessor)
		{
			continue;
		}

		auto [data, dataStride] = CgltfBufferAccessor(accessorInfo.Accessor, accessorInfo.DefaultStride);
		if (i == kPosition)
		{
			// Do bound box calculations as we read it in.
			DirectX::XMFLOAT3* posDst = vertexBufferBuilder.Place<DirectX::XMFLOAT3>(accessorInfo.Offset, vertexCount);
			const DirectX::XMFLOAT3* posSrc = reinterpret_cast<const DirectX::XMFLOAT3*>(data);

			DirectX::XMFLOAT3 minBounds = DirectX::XMFLOAT3(math::cMaxFloat, math::cMaxFloat, math::cMaxFloat);
			DirectX::XMFLOAT3 maxBounds = DirectX::XMFLOAT3(math::cMinFloat, math::cMinFloat, math::cMinFloat);

			for (int i = 0; i < vertexCount; i++)
			{
				*posDst = *posSrc;
				minBounds = math::Min(minBounds, *posDst);
				maxBounds = math::Max(maxBounds, *posDst);

				posDst++;
				posSrc++;
			}

			outPrim.BBoxLS = AABB(minBounds, maxBounds);
			outPrim.BoundsLS = Sphere(minBounds, maxBounds);
			
			DirectX::XMVECTOR boundsCoord = DirectX::XMLoadFloat3(&minBounds);
			DirectX::XMVECTOR result = DirectX::XMVector3TransformCoord(boundsCoord, localToObject);
			DirectX::XMStoreFloat3(&minBounds, result);

			boundsCoord = DirectX::XMLoadFloat3(&maxBounds);
			result = DirectX::XMVector3TransformCoord(boundsCoord, localToObject);
			DirectX::XMStoreFloat3(&maxBounds, result);

			outPrim.BBoxOS = AABB(minBounds, maxBounds);
			outPrim.BoundsOS = Sphere(minBounds, maxBounds);
		}
		else
		{
			vertexBufferBuilder.Place(accessorInfo.Offset, data, dataStride * vertexCount);
		}
	}

	outPrim.MaterialIdx;
	outPrim.PrimCount = indexCount;
}