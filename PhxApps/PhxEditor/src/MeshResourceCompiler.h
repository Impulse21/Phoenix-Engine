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

	enum VertexStreamType
	{
		Position = 0,
		Normal,
		Tangents,
		Uvset_0,
		Uvset_1,

		Count,
	};

	struct VertexStream
	{
		VertexStreamType Type;
		std::unique_ptr<uint8_t[]> Data;
		size_t ElementStride;
		size_t NumElements;

		VertexStream(VertexStreamType type, size_t elementStride, size_t numElements)
			: Type(type)
			, ElementStride(elementStride)
			, NumElements(numElements)
		{
			Data = std::make_unique<uint8_t[]>(elementStride * NumElements);
		}

		operator uint8_t* () { return Data.get(); }
		operator const uint8_t*() const { return Data.get(); }

		template<class T>
		T* As()
		{
			PHX_ASSERT(ElementStride == sizeof(T));
			return reinterpret_cast<T*>(Data.get());
		}

		template<class T>
		phx::Span<T> AsSpan() const
		{
			PHX_ASSERT(ElementStride == sizeof(T));
			return phx::Span( reinterpret_cast<T*>(Data.get()), NumElements);
		}

		template<class T>
		phx::SpanMutable<T> AsSpanMutable()
		{
			PHX_ASSERT(ElementStride == sizeof(T));
			return phx::SpanMutable(reinterpret_cast<T*>(Data.get()), NumElements);
		}
	};

	struct MeshData final
	{
		UUID ID;
		std::string Name;
		std::array<std::optional<VertexStream>, VertexStreamType::Count> VertexStreams;

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
		std::vector<uint32_t> ShadowIndices;


		std::vector<uint8_t> GpuBufferData;
		struct GeometryData
		{
			phx::StringHash MaterialId = {};
			uint32_t IndexOffset = 0;
			uint32_t IndexCount = 0;
		};
		std::vector<GeometryData> Geometry;

		inline size_t GetVertexCount() const
		{
			return Vertex_Positions.size();
		}

		template<typename T>
		VertexStream& AddVertexStream(VertexStreamType type, size_t numElements)
		{
			return VertexStreams[type].emplace(type, sizeof(T), numElements);
		}

		VertexStream* GetVertexStream(VertexStreamType type)
		{
			return VertexStreams[type].has_value()
				? &VertexStreams[type].value()
				: nullptr;
		}

		const VertexStream* GetVertexStream(VertexStreamType type) const
		{
			return VertexStreams[type].has_value()
				? &VertexStreams[type].value()
				: nullptr;
		}

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