#include "ModelInfoBuilder.h"

#include <meshoptimizer.h>
#include <PhxEngine/Core/Memory.h>

using namespace DirectX;

namespace
{
#if 0
	class MeshOptmizer
	{
	public:
		void OptmizeMesh()
		{
			// TODO: Follow the steps here https://github.com/zeux/meshoptimizer

		}

	private:
		void RemapIndices(MeshInfo& meshInfo)
		{
			PhxEngine::Core::FlexArray<meshopt_Stream> vertexStreams;
			vertexStreams.reserve(cMaxNumVertexStreams);
			vertexStreams.push_back({ &meshInfo.VertexStreams.Positions.data()->x, sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3)});
			if (EnumHasAllFlags(meshInfo.VertexFlags, VertexStreamFlags::kContainsTexCoords))
			{
				vertexStreams.push_back({ &meshInfo.TexCoords.data()->x, sizeof(DirectX::XMFLOAT2), sizeof(DirectX::XMFLOAT2)});
			}

			if (EnumHasAllFlags(meshInfo.VertexFlags, VertexStreamFlags::kContainsNormals))
			{
				vertexStreams.push_back({ &meshInfo.Normals.data()->x, sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3) });
			}

			if (EnumHasAllFlags(meshInfo.VertexFlags, VertexStreamFlags::kContainsTangents))
			{
				vertexStreams.push_back({ &meshInfo.Tangents.data()->x, sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3) });
			}

			PhxEngine::Core::FlexArray<unsigned int> remapIndices(meshInfo.Indices.size()); // allocate temporary memory for the remap table


			size_t vertexCount = meshopt_generateVertexRemapMulti(
				remapIndices.data(),
				meshInfo.Indices.data(),
				meshInfo.Indices.size(),
				meshInfo.VertexStreams.Positions.size(),
				vertexStreams.data(),
				vertexStreams.size());
		}
	};
#endif

}

MeshInfo& MeshInfo::OptmizeMesh()
{
	return *this;
}

MeshInfo& MeshInfo::ComputeBounds()
{
	// Calculate AABB
	DirectX::XMFLOAT3 minBounds = DirectX::XMFLOAT3(Math::cMaxFloat, Math::cMaxFloat, Math::cMaxFloat);
	DirectX::XMFLOAT3 maxBounds = DirectX::XMFLOAT3(Math::cMinFloat, Math::cMinFloat, Math::cMinFloat);

	if (!this->VertexStreams.Positions.empty())
	{
		for (int i = 0; i < this->GetTotalVertices(); i++)
		{
			minBounds = Math::Min(minBounds, this->VertexStreams.Positions[i]);
			maxBounds = Math::Max(maxBounds, this->VertexStreams.Positions[i]);
		}
	}

	this->BoundingBox = AABB(minBounds, maxBounds);
	this->BoundingSphere = Sphere(minBounds, maxBounds);
	return *this;
}

MeshInfo& MeshInfo::ComputeTangentSpace()
{
	if (!EnumHasAllFlags(this->VertexStreamFlags, (VertexStreamFlags::kContainsNormals | VertexStreamFlags::kContainsTexCoords | VertexStreamFlags::kContainsTangents)))
	{
		return *this;
	}

	Core::FlexArray<DirectX::XMVECTOR> computedTangents(this->GetTotalVertices());
	Core::FlexArray<DirectX::XMVECTOR> computedBitangents(this->GetTotalVertices());

	for (int i = 0; i < this->GetTotalIndices(); i += 3)
	{
		auto& index0 = this->Indices[i + 0];
		auto& index1 = this->Indices[i + 1];
		auto& index2 = this->Indices[i + 2];

		// Vertices
		DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&this->VertexStreams.Positions[index0]);
		DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&this->VertexStreams.Positions[index1]);
		DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&this->VertexStreams.Positions[index2]);

		// UVs
		DirectX::XMVECTOR uvs0 = DirectX::XMLoadFloat2(&this->VertexStreams.TexCoords[index0]);
		DirectX::XMVECTOR uvs1 = DirectX::XMLoadFloat2(&this->VertexStreams.TexCoords[index1]);
		DirectX::XMVECTOR uvs2 = DirectX::XMLoadFloat2(&this->VertexStreams.TexCoords[index2]);

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

	for (int i = 0; i < this->GetTotalVertices(); i++)
	{
		const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&this->VertexStreams.Normals[i]);
		const DirectX::XMVECTOR& tangent = computedTangents[i];
		const DirectX::XMVECTOR& bitangent = computedBitangents[i];

		// Gram-Schmidt orthogonalize
		DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
		float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
			? -1.0f
			: 1.0f;

		orthTangent = DirectX::XMVectorSetW(orthTangent, sign);
		DirectX::XMStoreFloat4(&this->VertexStreams.Tangents[i], orthTangent);
	}

	return *this;
}

MeshInfo& MeshInfo::GenerateMeshletData(size_t maxVertices, size_t maxTris, float coneWeight)
{
	MeshInfo& meshInfo = *this;
	size_t maxMeshlets = meshopt_buildMeshletsBound(meshInfo.Indices.size(), maxVertices, maxTris);
	meshInfo.MeshletData.Meshlets.resize(maxMeshlets);
	meshInfo.MeshletData.MeshletVertices.resize(maxMeshlets * maxVertices);
	meshInfo.MeshletData.MeshletTriangles.resize(maxMeshlets * maxTris * 3);

	
	size_t meshletCount = meshopt_buildMeshlets(
		meshInfo.MeshletData.Meshlets.data(),
		meshInfo.MeshletData.MeshletVertices.data(),
		meshInfo.MeshletData.MeshletTriangles.data(),
		meshInfo.Indices.data(),
		meshInfo.Indices.size(),
		&meshInfo.VertexStreams.Positions[0].x,
		meshInfo.VertexStreams.Positions.size(),
		sizeof(meshInfo.VertexStreams.Positions.front()),
		maxVertices,
		maxTris,
		coneWeight);

	const meshopt_Meshlet& last = meshInfo.MeshletData.Meshlets[meshletCount - 1];
	meshInfo.MeshletData.MeshletVertices.resize(last.vertex_offset + last.vertex_count);
	meshInfo.MeshletData.MeshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
	meshInfo.MeshletData.Meshlets.resize(meshletCount);

	meshInfo.MeshletData.MeshletBounds.reserve(meshletCount);
	for (meshopt_Meshlet& m : meshInfo.MeshletData.Meshlets)
	{
		meshopt_Bounds bounds = meshopt_computeMeshletBounds(
			&meshInfo.MeshletData.MeshletVertices[m.vertex_offset],
			&meshInfo.MeshletData.MeshletTriangles[m.triangle_offset],
			m.triangle_count,
			&meshInfo.VertexStreams.Positions.front().x,
			meshInfo.VertexStreams.Positions.size(),
			sizeof(meshInfo.VertexStreams.Positions.front()));

		meshInfo.MeshletData.MeshletBounds.push_back(bounds);
	}

	return meshInfo;
}