#pragma once

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/RHI/PhxRHI.h>

#include <memory>
#include <string>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	struct Texture
	{
		std::string Path;
		bool ForceSRGB;
		RHI::TextureHandle RhiTexture;

		std::shared_ptr<Core::IBlob> Data;
		std::vector<RHI::SubresourceData> SubresourceData;
	};

	struct Material
	{
		std::string Name;
		DirectX::XMFLOAT3 Albedo = { 0.0f, 0.0f, 0.0f };
		std::shared_ptr<Texture> AlbedoTexture;

		bool UseMaterialTexture = true;
		std::shared_ptr<Texture> MaterialTexture;

		std::shared_ptr<Texture> RoughnessTexture;
		std::shared_ptr<Texture> MetalnessTexture;
		std::shared_ptr<Texture> AoTexture;

		float Metalness = 0.0f;
		float Roughness = 0.0f;
		float Ao = 0.3f;

		std::shared_ptr<Texture> NormalMapTexture;

		// TODO: Add emissive Support
		// TODO: Add Occlusion data
		// TODO: Add Alpha data

		bool IsDoubleSided = false;
	};

	struct MeshGeometry
	{
		std::shared_ptr<Material> Material;
		uint32_t IndexOffsetInMesh = 0;
		uint32_t VertexOffsetInMesh = 0;
		uint32_t NumVertices = 0;
		uint32_t NumIndices = 0;
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
	};
}