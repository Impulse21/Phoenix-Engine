#pragma once

#include <memory>

#include <PhxCore/UUID.h>
#include <PhxCore/StringHash.h>
#include <PhxRenderer/MeshResource.h>
#include "CompiledResource.h"

#include <vector>
#include <DirectXMath.h>
#include <string>


namespace phxed
{
	class IBlob;

	struct MeshData final
	{
		UUID ID;
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

	
	class MeshResourceCompiler final
	{
	public:
		static void Compile(MeshData const& meshData, CompiledResource& outCompiledResource)
		{
			MeshResourceCompiler resourceCompiler(meshData, outCompiledResource);
			resourceCompiler.Compile();
		}

	private:
		MeshResourceCompiler(MeshData const& meshData, CompiledResource& outCompiledResource)
			: m_meshData(meshData)
			, m_outCompiledResource(outCompiledResource)
		{
		}

		void Compile();

		void BuildGpuBufferData(std::vector<uint8_t>& gpuBuffer) const;
		void BuildVertexBuffer(std::vector<uint8_t>& gpuBuffer) const;

	private:
		const MeshData& m_meshData;
		CompiledResource& m_outCompiledResource;
	};
}