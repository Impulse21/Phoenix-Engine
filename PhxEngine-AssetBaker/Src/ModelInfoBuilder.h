#pragma once

#include <DirectXMath.h>
#include <meshoptimizer.h>
#include <array>

#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/EnumClassFlags.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/Span.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace DirectX;

enum class VertexStreamFlags
{
	kEmpty = 0,
	kContainsNormals = 1 << 0,
	kContainsTexCoords = 1 << 1,
	kContainsTexCoords1 = 1 << 2,
	kContainsTangents = 1 << 3,
	kContainsColour = 1 << 4,
};
constexpr size_t cMaxNumVertexStreams = 5;
PHX_ENUM_CLASS_FLAGS(VertexStreamFlags);

struct VertexStreams
{
	PhxEngine::Core::FlexArray<DirectX::XMFLOAT3> Positions;
	PhxEngine::Core::FlexArray<DirectX::XMFLOAT2> TexCoords;
	PhxEngine::Core::FlexArray<DirectX::XMFLOAT2> TexCoords1;
	PhxEngine::Core::FlexArray<DirectX::XMFLOAT3> Normals;
	PhxEngine::Core::FlexArray<DirectX::XMFLOAT4> Tangents;
	PhxEngine::Core::FlexArray<DirectX::XMFLOAT3> Colour;
};

struct MeshPartInfo
{
	std::string Material;
	uint32_t IndexOffset;
	uint32_t IndexCount;
};

struct MeshInfo
{
	std::string_view Name;
	PhxEngine::Core::FlexArray<MeshPartInfo> MeshPart;
	VertexStreams VertexStreams;
	VertexStreamFlags VertexStreamFlags = VertexStreamFlags::kEmpty;
	PhxEngine::Core::FlexArray<uint32_t> Indices;
	Core::Sphere BoundingSphere;
	Core::AABB BoundingBox;

	MeshInfo(std::string_view const& name)
		: Name(name)
	{

	}
	struct MeshletData
	{
		PhxEngine::Core::FlexArray<meshopt_Meshlet> Meshlets;
		PhxEngine::Core::FlexArray<uint32_t> MeshletVertices;
		PhxEngine::Core::FlexArray<uint8_t> MeshletTriangles;
		PhxEngine::Core::FlexArray<meshopt_Bounds> MeshletBounds;

	} MeshletData;

	size_t GetTotalVertices() const { return this->VertexStreams.Positions.size(); }
	size_t GetTotalIndices() const { return this->Indices.size(); }
	MeshInfo& AddVertexStreamSpace(size_t vertexCount)
	{
		const size_t currentSpace = this->VertexStreams.Positions.size();
		const size_t newSpace = currentSpace + vertexCount;
		this->VertexStreams.Positions.resize(newSpace, {});
		this->VertexStreams.TexCoords.resize(newSpace, {});
		this->VertexStreams.TexCoords1.resize(newSpace, {});
		this->VertexStreams.Normals.resize(newSpace, {});
		this->VertexStreams.Tangents.resize(newSpace, {});
		this->VertexStreams.Colour.resize(newSpace, {});
		return *this;
	}
	MeshInfo& AddIndexSpace(size_t indexCount)
	{
		const size_t currentSpace = this->Indices.size();
		const size_t newSpace = currentSpace + indexCount;
		this->Indices.resize(newSpace, {});
	}

	MeshInfo& ComputeTangentSpace();
	MeshInfo& OptmizeMesh();
	MeshInfo& ComputeBounds();
	MeshInfo& GenerateMeshletData(size_t maxVertices, size_t maxTris, float coneWeight);

	MeshInfo& ReverseWinding()
	{
		for (size_t i = 0; i < this->Indices.size(); i += 3)
		{
			std::swap(this->Indices[i + 1], this->Indices[i + 2]);
		}

		return *this;
	}
	MeshInfo& ReverseZ()
	{
		// Flip Z
		for (int i = 0; i < this->GetTotalVertices(); i++)
		{
			this->VertexStreams.Positions[i].z *= -1.0f;
		}
		for (int i = 0; i < this->GetTotalVertices(); i++)
		{
			this->VertexStreams.Normals[i].z *= -1.0f;
		}
		for (int i = 0; i < this->GetTotalVertices(); i++)
		{
			this->VertexStreams.Tangents[i].z *= -1.0f;
		}

		return *this;
	}
};

namespace MeshOperations
{

}