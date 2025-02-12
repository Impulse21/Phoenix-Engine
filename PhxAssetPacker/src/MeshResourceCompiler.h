#pragma once

#include <memory>

#include <PhxCore/StringHash.h>
#include <PhxRenderer/MeshResource.h>

#include <vector>
#include <DirectXMath.h>
#include <string>

namespace phx
{
	class IBlob;

	struct MeshData final
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


		std::vector<uint8_t> GpuBufferData;
		struct GeometryData
		{
			phx::StringHash MaterialId = {};
			uint32_t IndexOffset = 0;
			uint32_t IndexCount = 0;
		};
		std::vector<GeometryData> Geometry;
	};

	struct CompiledMeshResource final
	{
		std::unique_ptr<IBlob> File;
	};

	constexpr uint32_t MshVersion = 1;
	class MeshResourceCompiler final
	{
	public:
		static void Compile(MeshData const& meshData, CompiledMeshResource& outCompiledMesh)
		{
			MeshResourceCompiler resourceCompiler(meshData, outCompiledMesh);
			resourceCompiler.Compile();
		}

	private:
		MeshResourceCompiler(MeshData const& meshData, CompiledMeshResource& outCompiledMesh)
			: m_meshData(meshData)
			, m_outCompiledMesh(outCompiledMesh)
		{
		}

		void Compile();

		void BuildGpuBufferData(std::vector<uint8_t>& gpuBuffer) const;
		void BuildVertexBuffer(std::vector<uint8_t>& gpuBuffer) const;

	private:
		const MeshData& m_meshData;
		CompiledMeshResource& m_outCompiledMesh;
	};
}