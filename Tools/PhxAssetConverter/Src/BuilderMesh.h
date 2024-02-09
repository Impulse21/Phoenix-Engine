#pragma once
#include <array>
#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Core/FlexArray.h>
namespace PhxEngine
{
	enum class VertexStreamType
	{
		Position = 0,
		Normals,
		TexCoords,
		TexCoords1,
		Tangents,
		Colour,
		NumStreams,
	};

	struct VertexStream
	{
		VertexStreamType Type;
		PhxEngine::Core::FlexArray<float> Data;
		size_t stride;
	};

	struct MeshPart
	{
		std::string Material;
		uint32_t IndexOffset;
		uint32_t IndexCount;
	};

	class BuilderMesh
	{
		std::string_view Name;
		PhxEngine::Core::FlexArray<MeshPart> MeshPart;
		std::array<VertexStream, static_cast<size_t>(VertexStreamType::NumStreams)> VertexStreams;
		PhxEngine::Core::FlexArray<uint32_t> Indices;

		Core::Sphere BoundingSphere;
		Core::AABB BoundingBox;
	};
}

