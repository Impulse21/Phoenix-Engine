#pragma once

#include <string>
#include <array>
#include <memory>

#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/Primitives.h>

namespace PhxEngine::Core
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
		PhxEngine::Core::FlexArray<float> Data;
		size_t NumComponents;

		bool IsEmpty() const { return this->Data.empty(); }
	};

	struct MeshPart
	{
		size_t MaterialHandle = ~0u;
		std::array<VertexStream, static_cast<size_t>(VertexStreamType::NumStreams)> VertexStreams;
		PhxEngine::Core::FlexArray<uint32_t> Indices;
	};

	struct Mesh
	{
		std::string_view Name;
		Core::FlexArray<MeshPart> MeshParts;
		Core::Sphere BoundingSphere;
		Core::AABB BoundingBox;
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

	struct Texture
	{
		std::string Name;
		std::string DataFile;
		std::string MimeType;
		std::unique_ptr<Core::IBlob> DataBlob;
		// TextureInfo
	};
}