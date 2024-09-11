#include "phxMeshConvert.h"

#include <array>

#include <Core/phxLog.h>
#include <Core/phxBinaryBuilder.h>
#include <Core/phxMath.h>
#include <Renderer/phxShaderInterop.h>
#include <Resource/phxResource.h>

#include <cgltf/cgltf.h>

#include <mesh-optimizer/meshoptimizer.h>

using namespace phx;
using namespace phx::MeshConverter;
using namespace phx::renderer;

namespace
{
	template<typename T>
	void FillVertexBuffer(BinaryBuilder& builder, size_t offset, T* src, size_t count)
	{
		if (!src)
			return;

		auto* data = builder.Place<T>(offset, count);
		std::memcpy(
			data,
			src,
			sizeof(T) * count);
	}

	template<typename T>
	void ComputeTangentSpace(DirectX::XMFLOAT3* positions, DirectX::XMFLOAT2* texcoords, T* indices, size_t indexCount, size_t vertexCount, std::unique_ptr<DirectX::XMFLOAT4[]>& outTangents)
	{
		std::vector<DirectX::XMVECTOR> computedTangents(vertexCount);
		std::vector<DirectX::XMVECTOR> computedBitangents(indexCount);

		for (int i = 0; i < this->TotalIndices; i += 3)
		{
			auto& index0 = indices[i + 0];
			auto& index1 = indices[i + 1];
			auto& index2 = indices[i + 2];

			// Vertices
			DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&positions[index0]);
			DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&positions[index1]);
			DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&positions[index2]);

			// UVs
			DirectX::XMVECTOR uvs0 = DirectX::XMLoadFloat2(&this->TexCoords[index0]);
			DirectX::XMVECTOR uvs1 = DirectX::XMLoadFloat2(&this->TexCoords[index1]);
			DirectX::XMVECTOR uvs2 = DirectX::XMLoadFloat2(&this->TexCoords[index2]);

			DirectX::XMVECTOR deltaPos1 = DirectX::XMVectorSubtract(pos1, pos0);
			DirectX::XMVECTOR deltaPos2 = DirectX::XMVectorSubtract(pos2, pos0);

			DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uvs1, uvs0);
			DirectX::XMVECTOR deltaUV2 = DirectX::XMVectorSubtract(uvs2, uvs0);

			// TODO: Take advantage of SIMD better here
			float r = 1.0f / (DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(deltaUV2) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(deltaUV2));

			DirectX::XMVECTOR tangent = (deltaPos1 * DirectX::XMVectorGetY(deltaUV2) - deltaPos2 * DirectX::XMVectorGetY(deltaUV1)) * r;
			DirectX::XMVECTOR bitangent = (deltaPos2 * DirectX::XMVectorGetX(deltaUV1) - deltaPos1 * DirectX::XMVectorGetX(deltaUV2)) * r;

			computedTangents[index0] += tangent;
			computedTangents[index1] += tangent;
			computedTangents[index2] += tangent;

			computedBitangents[index0] += bitangent;
			computedBitangents[index1] += bitangent;
			computedBitangents[index2] += bitangent;
		}

		outTangents.reset(new DirectX::XMFLOAT4[vertexCount]);
		for (int i = 0; i < indexCount; i++)
		{
			const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&this->Normals[i]);
			const DirectX::XMVECTOR& tangent = computedTangents[i];
			const DirectX::XMVECTOR& bitangent = computedBitangents[i];

			// Gram-Schmidt orthogonalize
			DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
			float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
				? -1.0f
				: 1.0f;

			orthTangent = DirectX::XMVectorSetW(orthTangent, sign);
			DirectX::XMStoreFloat4(&outTangents[i], orthTangent);
		}
	}

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

	if (inPrim.indices)
	{
		indexCount = inPrim.indices->count;
		outPrim.NumIndices = indexCount;

		// copy the indices
		auto [indexSrc, indexSrcStride] = CgltfBufferAccessor(inPrim.indices, 0);
		const size_t maxIndex = FindMaxIndex(inPrim.indices);

		b32BitIndices = maxIndex > 0xFFFF;
		const size_t indexSize = b32BitIndices ? 4 : 2;
		outPrim.IndexBuffer = std::make_shared<std::vector<uint8_t>>(std::vector<uint8_t>(indexSize * indexCount));


		// TODO: Need to fill in the vertex buffer.
		uint8_t* indexDst = outPrim.IndexBuffer->data();
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
			indexDst += indexSize;
		}
	}
	else
	{
		indexCount = vertexCount;
		outPrim.NumIndices = indexCount;
		b32BitIndices = indexCount > 0xFFFF;
		const size_t indexSize = b32BitIndices ? 4 : 2;
		outPrim.IndexBuffer = std::make_shared<std::vector<uint8_t>>(std::vector<uint8_t>(indexSize * indexCount));

		if (b32BitIndices)
		{
			uint32_t* indexDst = reinterpret_cast<uint32_t*>(outPrim.IndexBuffer->data());
			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				*indexDst = (uint32_t)iIdx;
				indexDst++;
			}
		}
		else
		{
			uint16_t* indexDst = reinterpret_cast<uint16_t*>(outPrim.IndexBuffer->data());
			for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
			{
				*indexDst = (uint16_t)iIdx;
				indexDst++;
			}
		}
	}

	std::unique_ptr<DirectX::XMFLOAT3[]> positions;
	std::unique_ptr<DirectX::XMFLOAT3[]> normal;
	std::unique_ptr<DirectX::XMFLOAT4[]> tangent;
	std::unique_ptr<DirectX::XMFLOAT2[]> texcoord0;
	std::unique_ptr<DirectX::XMFLOAT2[]> texcoord1;
	std::unique_ptr<DirectX::XMFLOAT3[]> color;
	std::unique_ptr<DirectX::XMFLOAT4[]> joints;
	std::unique_ptr<DirectX::XMFLOAT4[]> weights;
	outPrim.VertexSizeInBytes = 0;
	positions.reset(new DirectX::XMFLOAT3[vertexCount]);

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
			auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
			const DirectX::XMFLOAT3* posSrc = reinterpret_cast<const DirectX::XMFLOAT3*>(data);

			DirectX::XMFLOAT3 minBounds = DirectX::XMFLOAT3(math::cMaxFloat, math::cMaxFloat, math::cMaxFloat);
			DirectX::XMFLOAT3 maxBounds = DirectX::XMFLOAT3(math::cMinFloat, math::cMinFloat, math::cMinFloat);

			for (int i = 0; i < vertexCount; i++)
			{
				positions[i] = *posSrc;
				minBounds = math::Min(minBounds, positions[i]);
				maxBounds = math::Max(maxBounds, positions[i]);
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
			break;

		case cgltf_attribute_type_tangent:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 4;
			auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
			tangent.reset(new DirectX::XMFLOAT4[vertexCount]);
			std::memcpy(
				&tangent[0],
				data,
				dataStride*vertexCount);
			break;

		case cgltf_attribute_type_normal:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 3;
			auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
			normal.reset(new DirectX::XMFLOAT3[vertexCount]);
			std::memcpy(
				&normal[0],
				data,
				dataStride * vertexCount);
			break;

		case cgltf_attribute_type_texcoord:
			if (std::strcmp(cgltfAttribute.name, "TEXCOORD_0") == 0)
			{
				assert(cgltfAttribute.data->type == cgltf_type_vec2);
				assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
				const size_t dataStride = sizeof(float) * 2;
				auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
				texcoord0.reset(new DirectX::XMFLOAT2[vertexCount]);
				std::memcpy(
					&texcoord0[0],
					data,
					dataStride* vertexCount);
			}
			else if (std::strcmp(cgltfAttribute.name, "TEXCOORD_1") == 0)
			{
				assert(cgltfAttribute.data->type == cgltf_type_vec2);
				assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
				const size_t dataStride = sizeof(float) * 2;
				auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
				texcoord1.reset(new DirectX::XMFLOAT2[vertexCount]);
				std::memcpy(
					&texcoord1[0],
					data,
					dataStride * vertexCount);
			}
			break;
		case cgltf_attribute_type_color:
			assert(cgltfAttribute.data->type == cgltf_type_vec3);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 3;
			auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
			color.reset(new DirectX::XMFLOAT3[vertexCount]);
			std::memcpy(
				&color[0],
				data,
				dataStride * vertexCount);
			break;
		case cgltf_attribute_type_joints:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 4;
			auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
			joints.reset(new DirectX::XMFLOAT4[vertexCount]);
			std::memcpy(
				&joints[0],
				data,
				dataStride * vertexCount);
			break;
		case cgltf_attribute_type_weights:
			assert(cgltfAttribute.data->type == cgltf_type_vec4);
			assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
			const size_t dataStride = sizeof(float) * 4;
			auto [data, dataStride] = CgltfBufferAccessor(cgltfAttribute.data, dataStride);
			weights.reset(new DirectX::XMFLOAT4[vertexCount]);
			std::memcpy(
				&weights[0],
				data,
				dataStride * vertexCount);
			break;
		}
	}

	if (!normal)
	{
		PHX_ERROR("Prim doesn't contain normals");
		// TODO: Compute Normals
	}

	if (!tangent)
	{
		assert(indexCount % 3 == 0);
		if (texcoord0 && inPrim.material && inPrim.material->normal_texture.texcoord == 0)
		{
			if (b32BitIndices)
				ComputeTangentSpace<uint32_t>(positions.get(), texcoord0.get(), (uint32_t*)outPrim.IndexBuffer.get(), indexCount, vertexCount, tangent);
			else
				ComputeTangentSpace(positions.get(), texcoord0.get(), (uint16_t*)outPrim.IndexBuffer.get(), indexCount, vertexCount, tangent);
		}
		if (texcoord1 && inPrim.material && inPrim.material->normal_texture.texcoord == 1)
		{
			if (b32BitIndices)
				ComputeTangentSpace<uint32_t>(positions.get(), texcoord1.get(), (uint32_t*)outPrim.IndexBuffer.get(), indexCount, vertexCount, tangent);
			else
				ComputeTangentSpace(positions.get(), texcoord1.get(), (uint16_t*)outPrim.IndexBuffer.get(), indexCount, vertexCount, tangent);
		}
	}

	{
		BinaryBuilder vertexBufferBuilder;
		auto headerOfset = vertexBufferBuilder.Reserve<VertexStreamsHeader>();
		std::array<size_t, kNumStreams> streamOffsets(0);

		streamOffsets[kPosition] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT3>(vertexCount);
		outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT3) * vertexCount;
		streamOffsets[kNormals] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT3>(vertexCount);
		outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT3) * vertexCount;
		if (texcoord0)
		{
			streamOffsets[kUV0] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT2>(vertexCount);
			outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT2) * vertexCount;
		}
		if (texcoord1)
		{
			streamOffsets[kUV1] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT2>(vertexCount);
			outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT2) * vertexCount;
		}
		if (tangent)
		{
			streamOffsets[kTangents] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT4>(vertexCount);
			outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT4) * vertexCount;
		}
		if (color)
		{
			streamOffsets[kColour] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT3>(vertexCount);
			outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT3) * vertexCount;
		}
		if (joints && weights)
		{
			streamOffsets[kJoints] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT4>(vertexCount);
			outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT4) * vertexCount;

			streamOffsets[kWeights] = vertexBufferBuilder.Reserve<DirectX::XMFLOAT4>(vertexCount);
			outPrim.VertexSizeInBytes += sizeof(DirectX::XMFLOAT4) * vertexCount;
		}

		vertexBufferBuilder.Commit();
		auto* header = vertexBufferBuilder.Place<VertexStreamsHeader>(headerOfset);

		auto SetStreamDesc = [header, &streamOffsets](VertexStreamTypes type, size_t stride) {
			header->Desc[type].SetOffset(streamOffsets[type]);
			header->Desc[type].SetStride(stride);
			};

		SetStreamDesc(kPosition, sizeof(DirectX::XMFLOAT3));
		SetStreamDesc(kNormals, sizeof(DirectX::XMFLOAT3));
		SetStreamDesc(kUV0, sizeof(DirectX::XMFLOAT2));
		SetStreamDesc(kUV1, sizeof(DirectX::XMFLOAT2));
		SetStreamDesc(kTangents, sizeof(DirectX::XMFLOAT4));
		SetStreamDesc(kColour, sizeof(DirectX::XMFLOAT3));
		SetStreamDesc(kJoints, sizeof(DirectX::XMFLOAT4));
		SetStreamDesc(kWeights, sizeof(DirectX::XMFLOAT4));

		// fill data
		FillVertexBuffer<DirectX::XMFLOAT3>(vertexBufferBuilder, streamOffsets[kPosition], positions.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT3>(vertexBufferBuilder, streamOffsets[kNormals], normal.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT2>(vertexBufferBuilder, streamOffsets[kUV0], texcoord0.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT2>(vertexBufferBuilder, streamOffsets[kUV1], texcoord1.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT4>(vertexBufferBuilder, streamOffsets[kTangents], tangent.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT3>(vertexBufferBuilder, streamOffsets[kColour], color.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT4>(vertexBufferBuilder, streamOffsets[kJoints], joints.get(), vertexCount);
		FillVertexBuffer<DirectX::XMFLOAT4>(vertexBufferBuilder, streamOffsets[kWeights], weights.get(), vertexCount);

		outPrim.NumVertices = vertexCount;
		outPrim.VertexBuffer = vertexBufferBuilder.GetMemory();
	}

	if (inPrim.material->alpha_mode == cgltf_alpha_mode_blend)
		outPrim.PsoFlags |= PSOFlags::kAlphaBlend;

	if (inPrim.material->alpha_mode == cgltf_alpha_mode_mask)
		outPrim.PsoFlags |= PSOFlags::kAlphaTest;

	if (inPrim.material->double_sided)
		outPrim.PsoFlags |= PSOFlags::kTwoSided;

	outPrim.PrimCount = indexCount;
}