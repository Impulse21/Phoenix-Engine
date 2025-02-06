#pragma once

#include <PhxCore/ChunkFileFormat.h>
#include <PhxCore/StringHash.h>
#include <vector>
#include <DirectXMath.h>
#include <string>

namespace phx
{
	struct MeshData
	{
		std::string Name;
		std::vector<DirectX::XMFLOAT3> Vertex_Positions;
		std::vector<DirectX::XMFLOAT3> Vertex_Normals;
		std::vector<DirectX::XMFLOAT4> Vertex_Tangents;
		std::vector<DirectX::XMFLOAT2> Vertex_Uvset_0;
		std::vector<DirectX::XMFLOAT2> Vertex_Uvset_1;
#if false
		std::vector<DirectX::XMUINT4> Vertex_Boneindices;
		std::vector<DirectX::XMFLOAT4> Vertex_Boneweights;
		std::vector<DirectX::XMUINT4> Vertex_Boneindices2;
		std::vector<DirectX::XMFLOAT4> Vertex_Boneweights2;
		std::vector<DirectX::XMFLOAT2> Vertex_Atlas;
		std::vector<uint32_t> Vertex_Colors;
		std::vector<uint8_t> Vertex_Windweights;
#endif
		std::vector<uint32_t> Indices;

		struct GeometryData
		{
			phx::StringHash MaterialId = {};
			uint32_t IndexOffset = 0;
			uint32_t IndexCount = 0;
		};
		std::vector<GeometryData> Geometry;

	};

	namespace MeshBuilder
	{
		std::vector<uint8_t> Build(MeshData const& data, std::vector<C);
	}
}