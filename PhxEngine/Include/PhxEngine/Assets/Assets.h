#pragma once

#include <memory>
#include <PhxEngine/Core/Object.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/UUID.h>
#include <json.hpp>

namespace PhxEngine::Assets
{
    enum class AssetType
    {
        Unkown,
        Texture,
        Material,
        Mesh,
        Unknown,
    };

	class Asset : public Core::Object
	{
	public:
		Asset(AssetType type)
			: m_assetType(type)
		{}

		virtual ~Asset() = default;

		Core::UUID GetObjectId() { return this->m_objectId; } 
		Asset GetResourceType() const { return this->m_assetType; }

	public:
		template <typename T>
		static constexpr AssetType TypeToEnum();

	private:
		AssetType m_assetType;
		Core::UUID m_objectId;
	};

	using AssetRef = std::shared_ptr<Asset>;

	template<class T>
	using TypedAssetRef = std::shared_ptr<T>;

	class Material : public Asset
	{
	public:
		Material()
			: Asset(AssetType::Material)
		{}
	};

	using MaterialRef = std::shared_ptr<Material>;

	using MeshMetadata = nlohmann::json;
	class Mesh : public Asset
	{
	public:
		Mesh();
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
	using MeshRef = std::shared_ptr<Mesh>;
}