#pragma once

#include <PhxCore/StringHash.h>
#include "PhxRenderer/MeshResource.h"

#include <vector>
#include <DirectXMath.h>
#include <string>

namespace phx
{
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

		std::vector<uint8_t> Compile()
		{

			BinaryBuilder vertexBufferBuilder;
			auto headerOfset = vertexBufferBuilder.Reserve<VertexStreamsHeader>();
			std::array<size_t, kNumStreams> streamOffsets = {};


			vertexBufferBuilder.Commit();
			auto* header = vertexBufferBuilder.Place<VertexStreamsHeader>(headerOfset);

			auto SetStreamDesc = [header, &streamOffsets](VertexStreamTypes type, size_t stride) {
				header->Desc[type].SetOffset((uint)streamOffsets[type]);
				header->Desc[type].SetStride((uint)stride);
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

			outPrim.NumVertices = (uint32_t)vertexCount;
			outPrim.VertexBufferSize = (uint32_t)vertexBufferBuilder.Size();
			outPrim.VertexBuffer = vertexBufferBuilder.GetMemory();
		}

	private:
	};

	struct CompiledMeshResource final
	{
		phx::renderer::MeshCpuMetadata Metadata;
		std::vector<uint8_t> GpuData;
	};

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

		std::vector<uint8_t> BuildVertexBuffer();

	private:
		const MeshData& m_meshData;
		CompiledMeshResource& m_outCompiledMesh;
	};
}