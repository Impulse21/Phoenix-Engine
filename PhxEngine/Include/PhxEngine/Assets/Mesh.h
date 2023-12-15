#pragma once

#include <PhxEngine/Core/Object.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/RefCountPtr.h>
#include <json.hpp>

namespace PhxEngine::Assets
{
	class IAsset : public Core::Object
	{
	public:
		virtual ~IAsset() = default;
	};

	class Material : public Core::RefCounter<IAsset>
	{
	public:
		Material() = default;
	};

	using MaterialRef = Core::RefCountPtr<Material>;


	using MeshMetadata = nlohmann::json;
	class Mesh : public Core::RefCounter<IAsset>
	{
	public:
		Mesh(MeshMetadata const& metdata);

		struct MeshPart
		{
			MaterialRef Material;
			uint32_t IndexOffset;
			uint32_t IndexCount;
		};
	private:

		struct Geometry
		{
			uint32_t MaterialIndex;
			uint32_t NumIndices;
			uint32_t NumVertices;
			uint32_t IndexOffset;

			// -- 16 byte boundary ---
			uint32_t VertexOffset;
			uint32_t MeshletOffset;
			uint32_t MeshletCount;
			uint32_t MeshletPrimtiveOffset;

			// -- 16 byte boundary ---
			uint32_t MeshletUniqueVertexIBOffset;

		};

		PhxEngine::Core::FlexArray<Geometry> m_geometryData;

		struct GpuBufferOffsets
		{
			uint32_t IndexOffset;
			uint32_t VertexOffset;
			uint32_t MeshletOffset;
			uint32_t PrimitiveOffset;

			// -- 16 byte boundary ---
			uint32_t MeshletCullOffset;
		} m_bufferOffsets;
		

		Core::FlexArray<Geometry> m_geometry;
		size_t m_numGeometries;

		// index + vertex + meshlets + meshletCull data
		RHI::BufferHandle m_generalGpuBuffer;
	};
	using MeshRef = Core::RefCountPtr<Mesh>;
}

