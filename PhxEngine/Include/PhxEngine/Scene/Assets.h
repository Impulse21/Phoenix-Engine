#pragma once

#include <array>

#include <PhxEngine/Core/RefPtr.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <Shaders/ShaderInteropStructures.h>

// It would be good to hide these details somehow.
#include <DirectXMath.h>
#include "DirectXTex.h"

namespace PhxEngine::Graphics
{
	class TextureCache;
}

namespace PhxEngine::Scene::Assets
{
	class Asset //: public Core::RefCounter
	{
	public:
		Asset() = default;
		virtual ~Asset() = default;

		std::string Name;

	protected:
		virtual void CreateRenderResourceIfEmpty() = 0;
	};

	class Texture : public Asset
	{
		friend PhxEngine::Graphics::TextureCache;
	public:
		Texture();
		~Texture();

		RHI::TextureHandle GetRenderHandle() { return this->m_renderTexture; }

	protected:
		void CreateRenderResourceIfEmpty() override;

	private:
		RHI::TextureHandle m_renderTexture;

		std::string m_path;
		bool m_forceSRGB;
		// std::shared_ptr<Core::IBlob> Data;
		std::vector<RHI::SubresourceData> m_subresourceData;

		DirectX::TexMetadata m_metadata;
		DirectX::ScratchImage m_scratchImage;
	};

	class StandardMaterial : public Asset
	{
	public:
		StandardMaterial();
		~StandardMaterial() = default;

		enum Flags
		{
			kEmpty = 0,
			kCastShadow = 1 << 0,
		};

		uint32_t Flags = kEmpty;

		enum ShaderType : uint8_t
		{
			kPbr = 0,
			kUnlit = 1,
			kEmissive = 2,
			kNumShaderTypes
		} ShaderType = kPbr;

		PhxEngine::Renderer::BlendMode BlendMode = Renderer::BlendMode::Opaque;
		DirectX::XMFLOAT4 Albedo = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		float Metalness = 1.0f;
		float Roughness = 1.0f;
		float Ao = 1.0f;
		bool IsDoubleSided = false;

		std::shared_ptr<Assets::Texture> AlbedoTexture;
		std::shared_ptr<Assets::Texture> MetalRoughnessTexture;
		std::shared_ptr<Assets::Texture> AoTexture;
		std::shared_ptr<Assets::Texture> NormalMapTexture;

		size_t GlobalBufferIndex = 0;
		void PopulateShaderData(Shader::MaterialData& shaderData);
		uint32_t GetRenderTypes();

#if false
		void SetAlbedo(DirectX::XMFLOAT4 const& albedo);
		DirectX::XMFLOAT4 GetAlbedo();

		void SetEmissive(DirectX::XMFLOAT4 const& emissive);
		DirectX::XMFLOAT4 GetEmissive();

		void SetMetalness(float metalness);
		float GetMetalness();

		void SetRoughness(float roughness);
		float GetRoughtness();

		void SetAO(float ao);
		float GetAO();
		void SetDoubleSided(bool isDoubleSided);
		bool IsDoubleSided();

		void SetAlbedoTexture(std::shared_ptr<Texture>& texture);
		std::shared_ptr<Texture> GetAlbedoTexture();

		void SetMaterialTexture(std::shared_ptr<Texture>& texture);
		std::shared_ptr<Texture> GetMaterialTexture();

		void SetAoTexture(std::shared_ptr<Texture>& texture);
		std::shared_ptr<Texture> GetAoTexture();

		void SetNormalMapTexture(std::shared_ptr<Texture>& texture);
		std::shared_ptr<Texture> GetNormalMapTexture();

		void SetBlendMode(PhxEngine::Renderer::BlendMode blendMode);
		PhxEngine::Renderer::BlendMode GetBlendMode();
#endif
	protected:
		void CreateRenderResourceIfEmpty() override;
	};

	class Mesh : public Asset
	{
	public:
		Mesh();
		Mesh(std::string const& meshName);
		~Mesh();

		// I am to lazy - keep this public for now XD
		enum Flags
		{
			kEmpty = 0,
			kContainsNormals = 1 << 0,
			kContainsTexCoords = 1 << 1,
			kContainsTangents = 1 << 2,
		};

		uint32_t Flags = kEmpty;
		std::vector<DirectX::XMFLOAT3> VertexPositions;
		std::vector<DirectX::XMFLOAT2> VertexTexCoords;
		std::vector<DirectX::XMFLOAT3> VertexNormals;
		std::vector<DirectX::XMFLOAT4> VertexTangents;
		std::vector< DirectX::XMFLOAT3> VertexColour;
		std::vector<uint32_t> Indices;

		uint32_t TotalIndices = 0;
		uint32_t TotalVertices = 0;

		struct SurfaceDesc
		{
			std::shared_ptr<StandardMaterial> Material;
			uint32_t IndexOffsetInMesh = 0;
			uint32_t VertexOffsetInMesh = 0;
			uint32_t NumVertices = 0;
			uint32_t NumIndices = 0;
			size_t GlobalBufferIndex = 0;
		};

		std::vector<SurfaceDesc> Surfaces;

		enum class VertexAttribute : uint8_t
		{
			Position = 0,
			TexCoord,
			Normal,
			Tangent,
			Colour,
			Count,
		};

		std::array<RHI::BufferRange, (int)VertexAttribute::Count> BufferRanges;

		[[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
		RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
		[[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }

		RHI::BufferHandle VertexGpuBuffer;
		RHI::BufferHandle IndexGpuBuffer;

		// Utility Functions
		void ReverseWinding();
		void ComputeTangentSpace();

		bool IsTriMesh() const { return (this->Indices.size() % 3) == 0; }

		void CreateRenderData(RHI::CommandListHandle commandList);

	protected:
		void CreateRenderResourceIfEmpty() {};
	};

}

