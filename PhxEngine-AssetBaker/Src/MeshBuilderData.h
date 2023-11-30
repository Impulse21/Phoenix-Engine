#pragma once

#include <DirectXMath.h>
#include <meshoptimizer.h>

#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/EnumClassFlags.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Primitives.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace DirectX;

enum class MeshVertexFlags
{
	kEmpty = 0,
	kContainsNormals = 1 << 0,
	kContainsTexCoords = 1 << 1,
	kContainsTangents = 1 << 2,
	kContainsColour = 1 << 3,
};
PHX_ENUM_CLASS_FLAGS(MeshVertexFlags);

struct MeshBuildData
{
	enum RenderType
	{
		RenderType_Void = 0,
		RenderType_Opaque = 1 << 0,
		RenderType_Transparent = 1 << 1,
		RenderType_All = RenderType_Opaque | RenderType_Transparent
	};

	MeshVertexFlags VertexFlags = MeshVertexFlags::kEmpty;

	uint32_t TotalVertices = 0;
	DirectX::XMFLOAT3* Positions = nullptr;
	DirectX::XMFLOAT2* TexCoords = nullptr;
	DirectX::XMFLOAT3* Normals = nullptr;
	DirectX::XMFLOAT4* Tangents = nullptr;
	DirectX::XMFLOAT3* Colour = nullptr;

	uint32_t TotalIndices = 0;
	uint32_t* Indices = nullptr;

	meshopt_Meshlet* Meshlets = nullptr;
	uint32_t* MeshletVertices = nullptr;
	uint8_t* MeshletTriangles = nullptr;

	Core::Sphere BoundingSphere;
	Core::AABB BoundingBox;

	void ComputeTangentSpace()
	{
		Core::FlexArray<DirectX::XMVECTOR> computedTangents(this->TotalVertices);
		Core::FlexArray<DirectX::XMVECTOR> computedBitangents(this->TotalVertices);

		for (int i = 0; i < this->TotalIndices; i += 3)
		{
			auto& index0 = this->Indices[i + 0];
			auto& index1 = this->Indices[i + 1];
			auto& index2 = this->Indices[i + 2];

			// Vertices
			DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&this->Positions[index0]);
			DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&this->Positions[index1]);
			DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&this->Positions[index2]);

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

		for (int i = 0; i < this->TotalVertices; i++)
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
			DirectX::XMStoreFloat4(&this->Tangents[i], orthTangent);
		}
	}

	void OptmizeMesh(MeshBuildData& data);

	void ConstructMeshletData(MeshBuildData& data);
};