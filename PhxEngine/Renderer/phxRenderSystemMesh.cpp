#include "pch.h"
#include "phxRenderSystem.h"
#include "World/phxWorld.h"

#include "Core/phxMemory.h"
#include "RHI/PhxRHI.h"

using namespace phx;
using namespace DirectX;

namespace
{
	rhi::BufferHandle gVB_Cube;
	rhi::BufferHandle gIB_Cube;

	struct MeshDrawable
	{
		rhi::BufferHandle VertexBuffer;
		rhi::BufferHandle IndexBuffer;
	};

	void ComputeTangentSpace()
	{
		std::vector<DirectX::XMVECTOR> computedTangents(this->TotalVertices);
		std::vector<DirectX::XMVECTOR> computedBitangents(this->TotalVertices);

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

	void CreateCubeData(float size = 1)
	{

		// A cube has six faces, each one pointing in a different direction.
		constexpr int FaceCount = 6;

		constexpr XMVECTORF32 faceNormals[FaceCount] =
		{
			{ 0,  0,  1 },
			{ 0,  0, -1 },
			{ 1,  0,  0 },
			{ -1,  0,  0 },
			{ 0,  1,  0 },
			{ 0, -1,  0 },
		};

		constexpr XMFLOAT4 faceColour[] =
		{
			{ 1.0f,  0.0f,  0.0f, 1.0f },
			{ 0.0f,  1.0f,  0.0f, 1.0f },
			{ 0.0f,  0.0f,  1.0f, 1.0f },
		};

		constexpr XMFLOAT2 textureCoordinates[4] =
		{
			{ 1, 0 },
			{ 1, 1 },
			{ 0, 1 },
			{ 0, 0 },
		};

		size /= 2;
		const size_t totalVerts = FaceCount * 4;
		const size_t totalIndices = FaceCount * 6;

		BEGIN_TEMP_SCOPE;
		auto& allocator = Memory::GetScratchAllocator();

		const size_t positionSize = sizeof(DirectX::XMFLOAT3) * totalVerts;
		const size_t texCoordsSize = sizeof(DirectX::XMFLOAT2) * totalVerts;
		const size_t normalSize = sizeof(DirectX::XMFLOAT3) * totalVerts;
		const size_t tangentSize = sizeof(DirectX::XMFLOAT4) * totalVerts;
		const size_t colourSize = sizeof(DirectX::XMFLOAT4) * totalVerts;
		const size_t totalVBSize = positionSize + texCoordsSize + normalSize + tangentSize + colourSize + 1;

		// allocate VB + 1 byte for offset flags
		uint8_t* vbData = allocator.Alloc<uint8_t>(totalVBSize);
		*vbData = 0xFF;

		SpanMutable<DirectX::XMFLOAT3> positions(reinterpret_cast<DirectX::XMFLOAT3*>(vbData + 1), totalVerts);
		SpanMutable<DirectX::XMFLOAT2> texCoords(reinterpret_cast<DirectX::XMFLOAT2*>(positions.end()), totalVerts);
		SpanMutable<DirectX::XMFLOAT3> normals(reinterpret_cast<DirectX::XMFLOAT3*>(texCoords.end()), totalVerts);
		SpanMutable<DirectX::XMFLOAT4> tangents(reinterpret_cast<DirectX::XMFLOAT4*>(normals.end()), totalVerts);
		SpanMutable<DirectX::XMFLOAT4> colour(reinterpret_cast<DirectX::XMFLOAT4*>(tangents.end()), totalVerts);


		auto indices = allocator.AllocSpanMutable<uint32_t>(totalIndices);
		size_t vbase = 0;
		size_t ibase = 0;
		for (int i = 0; i < FaceCount; i++)
		{
			XMVECTOR normal = faceNormals[i];

			// Get two vectors perpendicular both to the face normal and to each other.
			XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

			XMVECTOR side1 = XMVector3Cross(normal, basis);
			XMVECTOR side2 = XMVector3Cross(normal, side1);

			// Six indices (two triangles) per face.
			indices[ibase + 0] = static_cast<uint32_t>(vbase + 0);
			indices[ibase + 1] = static_cast<uint32_t>(vbase + 1);
			indices[ibase + 2] = static_cast<uint32_t>(vbase + 2);

			indices[ibase + 3] = static_cast<uint32_t>(vbase + 0);
			indices[ibase + 4] = static_cast<uint32_t>(vbase + 2);
			indices[ibase + 5] = static_cast<uint32_t>(vbase + 3);

			XMFLOAT3 positon;
			XMStoreFloat3(&positon, (normal - side1 - side2) * size);
			positions[vbase + 0] = positon;

			XMStoreFloat3(&positon, (normal - side1 + side2) * size);
			positions[vbase + 1] = positon;

			XMStoreFloat3(&positon, (normal + side1 + side2) * size);
			positions[vbase + 2] = positon;

			XMStoreFloat3(&positon, (normal + side1 - side2) * size);
			positions[vbase + 3] = positon;

			texCoords[vbase + 0] = textureCoordinates[0];
			texCoords[vbase + 1] = textureCoordinates[1];
			texCoords[vbase + 2] = textureCoordinates[2];
			texCoords[vbase + 3] = textureCoordinates[3];

			colour[vbase + 0] = faceColour[0];
			colour[vbase + 1] = faceColour[1];
			colour[vbase + 2] = faceColour[2];
			colour[vbase + 3] = faceColour[3];

			DirectX::XMStoreFloat3(&normals[vbase + 0], normal);
			DirectX::XMStoreFloat3(&normals[vbase + 1], normal);
			DirectX::XMStoreFloat3(&normals[vbase + 2], normal);
			DirectX::XMStoreFloat3(&normals[vbase + 3], normal);

			vbase += 4;
			ibase += 6;
		}

		mesh.ComputeTangentSpace();

		if (rhsCoord)
		{
			mesh.ReverseWinding();
			mesh.FlipZ();
		}

		// Calculate AABB
		mesh.ComputeBounds();
		mesh.ComputeMeshletData(this->GetAllocator());

		mesh.BuildRenderData(this->GetAllocator(), gfxDevice);
		return retVal;
	}
}

phx::RenderSystemMesh::RenderSystemMesh()
{
	CreateCubeData();
}

phx::RenderSystemMesh::~RenderSystemMesh()
{
	if (gVB_Cube.IsValid())
		rhi::GfxDevice::Ptr->DeleteBuffer(gVB_Cube);

	if (gIB_Cube.IsValid())
		rhi::GfxDevice::Ptr->DeleteBuffer(gIB_Cube);
}

void* phx::RenderSystemMesh::CacheData(World const& world)
{
	const entt::registry& registry = world.GetRegistry();

	auto& allocator = Memory::GetFrameAllocator();

	allocator.AllocS
	
	return nullptr;
}