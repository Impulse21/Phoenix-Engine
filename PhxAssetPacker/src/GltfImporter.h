#pragma once

#include <PhxCore/StringHash.h>
#include <memory>
#include <vector>
#include <DirectXMath.h>
struct cgltf_data;
struct cgltf_mesh;

namespace phx
{
	struct MeshData
	{
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

	class GltfMeshImporter
	{
	public:
		static std::vector<MeshData> Import(cgltf_data* gltfData)
		{
			GltfMeshImporter importer(gltfData);
			return importer.Import();
		}

	private:
		GltfMeshImporter(cgltf_data* gltfData)
			: m_gltfData(gltfData)
		{}

		std::vector<MeshData> Import();

		void ProcessMesh(MeshData& meshData, cgltf_mesh const& cgltfMesh);
	private:
		cgltf_data* m_gltfData;
	};
}

