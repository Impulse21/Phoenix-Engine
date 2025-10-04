#pragma once

#include <memory>

#include <PhxCore/UUID.h>
#include <PhxCore/StringHash.h>
#include <PhxRenderer/MeshResource.h>
#include <PhxRenderer/shaders/ShaderInterop.h>
#include "CompiledResource.h"

#include <vector>
#include <DirectXMath.h>
#include <string>


namespace phxed
{
	class IBlob;

	struct VertexStream
	{
		phx::renderer::VertexStreamType Type;
		std::unique_ptr<uint8_t[]> Data;
		size_t ElementStride;
		size_t NumElements;

		VertexStream(phx::renderer::VertexStreamType type, size_t elementStride, size_t numElements)
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

	struct Mesh final
	{
		phx::UUID ID;
		std::string Name;
		std::array<std::optional<VertexStream>, phx::renderer::VertexStream_Count> VertexStreams;
		std::vector<uint32_t> Indices;
		std::vector<uint32_t> ShadowIndices;


		std::vector<uint8_t> GpuBufferData;
		struct GeometryData
		{
			uint32_t mat_assignment_id = {};
			bool is_indexed = false;
			uint32_t vertex_count;
			union
			{
				struct
				{
					uint32_t index_offset;
					uint32_t index_count;
				};
				struct
				{
					uint32_t vertex_offset;
				};
			};
		};
		std::vector<GeometryData> Geometry;

		inline size_t GetVertexCount() const
		{
			if (VertexStreams[phx::renderer::VertexStream_Position].has_value())
				return VertexStreams[phx::renderer::VertexStream_Position].value().NumElements;

			return 0;
		}

		template<typename T>
		VertexStream& AddVertexStream(phx::renderer::VertexStreamType type, size_t numElements)
		{
			return VertexStreams[type].emplace(type, sizeof(T), numElements);
		}

		template<typename T>
		VertexStream& AddVertexStream(phx::renderer::VertexStreamType type, size_t num_components, size_t num_elements)
		{
			return VertexStreams[type].emplace(type, sizeof(T) * num_components, num_elements);
		}

		VertexStream* GetVertexStream(phx::renderer::VertexStreamType type)
		{
			return VertexStreams[type].has_value()
				? &VertexStreams[type].value()
				: nullptr;
		}

		const VertexStream* GetVertexStream(phx::renderer::VertexStreamType type) const
		{
			return VertexStreams[type].has_value()
				? &VertexStreams[type].value()
				: nullptr;
		}
	};

	
	class MeshResourceCompiler final
	{
	public:
		static void Compile(Mesh const& meshData, CompiledResource& outCompiledResource)
		{
			MeshResourceCompiler resourceCompiler(meshData, &outCompiledResource);
			resourceCompiler.Compile();
		}

		static std::vector<uint8_t> BuildGpuBufferData(Mesh const& meshData)
		{
			MeshResourceCompiler resourceCompiler(meshData, nullptr);
			std::vector<uint8_t> ret_val;
			resourceCompiler.BuildGpuBufferData(ret_val);

			return ret_val;
		}

	private:
		MeshResourceCompiler(Mesh const& meshData, CompiledResource* /*outCompiledResource*/)
			: m_meshData(meshData)
			//, m_outCompiledResource(outCompiledResource)
		{
		}

		void Compile();

		void BuildGpuBufferData(std::vector<uint8_t>& gpuBuffer) const;
		void BuildVertexBuffer(std::vector<uint8_t>& gpuBuffer) const;

	private:
		const Mesh& m_meshData;
		//CompiledResource* m_outCompiledResource;
	};
}