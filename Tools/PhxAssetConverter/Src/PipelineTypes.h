#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>

#include <PhxEngine/Core/Primitives.h>

#include <meshoptimizer.h>


namespace PhxEngine
{
	class IBlob;
}

namespace PhxEngine::Pipeline
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
		std::vector<float> Data;
		size_t NumComponents;

		[[nodiscard]] bool IsEmpty() const { return this->Data.empty(); }
		[[nodiscard]] size_t GetNumElements() const { return this->Data.size() / NumComponents; }
		[[nodiscard]] size_t GetElementStride() const { return sizeof(float) * this->NumComponents; }
		operator float* () { return Data.data(); }
	};

	struct MeshPart
	{
		size_t MaterialHandle = ~0u;
		uint32_t IndexOffset = 0;
		uint32_t IndexCount = 0;

		std::vector<meshopt_Meshlet> Meshlets;
		std::vector<unsigned int> MeshletVertices;
		std::vector<unsigned char> MeshletTriangles;
		std::vector<meshopt_Bounds> MeshletBounds;
	};

	struct CompiledMesh
	{
		uint32_t Name;
		std::vector<uint8_t> GeometryData;
		struct Geometry
		{
			uint32_t Material;
			uint32_t IndexOffset = 0;
			uint32_t IndexCount = 0;
			uint32_t MeshletOffset = 0;
			uint32_t MeshletCount = 0;
		};

		std::vector<Geometry> Geometry;

		PhxEngine::Sphere BoundingSphere;
		PhxEngine::AABB BoundingBox;

		// Offsets
		uint32_t VBOffset;
		uint32_t VBSize;
		uint32_t IBOffset;
		uint32_t IBSize;
		uint32_t MeshletOffset;
		uint32_t MeshletSize;
		uint32_t MeshletVBOffset;
		uint32_t MeshletVBSize;
		uint32_t MeshletBoundsOffset;
		uint32_t MeshletBoundsSize;
		uint8_t IBFormat;
	};

	struct Mesh
	{
		std::string Name;
		std::vector<MeshPart> MeshParts;

		std::array<VertexStream, static_cast<size_t>(VertexStreamType::NumStreams)> VertexStreams;
		std::vector<uint32_t> Indices;

		PhxEngine::Sphere BoundingSphere;
		PhxEngine::AABB BoundingBox;

		Mesh()
		{
			this->GetStream(VertexStreamType::Position).NumComponents = 3;
			this->GetStream(VertexStreamType::Colour).NumComponents = 3;
			this->GetStream(VertexStreamType::Tangents).NumComponents = 4;
			this->GetStream(VertexStreamType::Normals).NumComponents = 3;
			this->GetStream(VertexStreamType::TexCoords).NumComponents = 2;
			this->GetStream(VertexStreamType::TexCoords1).NumComponents = 2;
		}

		[[nodiscard]] size_t GetNumVertices() const
		{
			return this->VertexStreams[static_cast<size_t>(Pipeline::VertexStreamType::Position)].GetNumElements();
		}

		[[nodiscard]] size_t HasIndices() const
		{
			return !this->Indices.empty();
		}

		[[nodiscard]] VertexStream& GetStream(VertexStreamType type)
		{
			return this->VertexStreams[static_cast<size_t>(type)];
		}
	};

	struct Material
	{
		enum class BlendMode
		{
			Opaque = 0,
			Alpha
		};

		enum class PbrWorkflow
		{
			Metallic_Roughness = 0,
			Specular_Glossy
		};

		std::string Name;
		BlendMode BlendMode = BlendMode::Opaque;
		size_t NormalMapHandle = ~0u;
		size_t EmissiveTextureHandle = ~0u;
		bool IsDoubleSided = false;

		PbrWorkflow Workflow = PbrWorkflow::Metallic_Roughness;
		std::array<float, 4> BaseColour;
		size_t BaseColourTextureHandle = ~0u;
		size_t MetalRoughnessTextureHandle = ~0u;
		float Metalness = 1.0f;
		float Roughness = 1.0f;

	};

	enum class TextureUsage
	{
		Default = 0,
		Albedo,
		NormalMap,
	};

	struct Texture
	{
		std::string Name;
		std::string DataFile;
		std::string MimeType;
		bool isDDS = false;
		bool forceSrgb = false;
		TextureUsage Usage = TextureUsage::Default;
		std::unique_ptr<IBlob> DataBlob;
		// TextureInfo
	};
}