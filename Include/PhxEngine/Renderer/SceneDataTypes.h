#pragma once

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/RHI/PhxRHI.h>

#include <array>
#include <memory>
#include <string>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	enum class VertexAttribute : uint8_t
	{
		Position = 0,
		TexCoord,
		Normal,
		Tangent,
		Colour,
		Count,
	};

	struct Material
	{
		std::string Name;
		size_t GlobalMaterialIndex = ~0U;

		DirectX::XMFLOAT3 Albedo = { 0.0f, 0.0f, 0.0f };
		RHI::TextureHandle AlbedoTexture;

		bool UseMaterialTexture = true;
		RHI::TextureHandle MaterialTexture;

		RHI::TextureHandle RoughnessTexture;
		RHI::TextureHandle MetalnessTexture;
		RHI::TextureHandle AoTexture;

		float Metalness = 0.0f;
		float Roughness = 0.0f;
		float Ao = 0.3f;

		RHI::TextureHandle NormalMapTexture;

		// TODO: Add emissive Support
		// TODO: Add Occlusion data
		// TODO: Add Alpha data

		// bool IsDoubleSided = false;
	};

	struct MeshGeometry
	{
		std::shared_ptr<Material> Material;
		uint32_t IndexOffsetInMesh = 0;
		uint32_t VertexOffsetInMesh = 0;
		uint32_t NumVertices = 0;
		uint32_t NumIndices = 0;
		size_t GlobalGeometryIndex = 0;
	};

	struct MeshBuffers
	{
		RHI::BufferHandle VertexBuffer;
		RHI::BufferHandle IndexBuffer;
		RHI::BufferHandle InstanceBuffer;

		std::vector<uint32_t> IndexData;
		std::vector<DirectX::XMFLOAT3> PositionData;
		std::vector<DirectX::XMFLOAT2> TexCoordData;
		std::vector<DirectX::XMFLOAT3> NormalData;
		std::vector<DirectX::XMFLOAT4> TangentData;
		std::vector< DirectX::XMFLOAT3> ColourData;

		std::array<RHI::BufferRange, (int)VertexAttribute::Count> BufferRanges;

		[[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
		RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
		[[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }
	};

	struct Mesh
	{
		std::string Name = "";
		std::vector<std::shared_ptr<MeshGeometry>> Geometry;
		std::shared_ptr<MeshBuffers> Buffers;

		uint32_t IndexOffset = 0;
		uint32_t VertexOffset = 0;
		uint32_t TotalIndices = 0;
		uint32_t TotalVertices = 0;
		size_t GlobalMeshIndex = 0;
	};
}