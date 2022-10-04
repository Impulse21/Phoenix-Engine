#pragma once

#include <deque>

#include <PhxEngine/Renderer/ResourceManager.h>
#include <Shaders/ShaderInteropStructures.h>

namespace PhxEngine::Renderer
{
	class Dx12Material
	{
		Shader::MaterialData Data = {};

		enum Flags
		{
			kEmpty = 0,
			kCastShadow = 1 << 0,
		};

		uint32_t Flags;

		enum ShaderType : uint8_t
		{
			kPbr = 0,
			kUnlit = 1,
			kEmissive = 2,
			kNumShaderTypes
		} ShaderType = kPbr;

		PhxEngine::Renderer::BlendMode BlendMode = PhxEngine::Renderer::BlendMode::Opaque;
		DirectX::XMFLOAT4 Albedo = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		float Metalness = 1.0f;
		float Roughness = 1.0f;
		float Ao = 1.0f;
		bool IsDoubleSided = false;

		RHI::TextureHandle AlbedoTexture;
		RHI::TextureHandle MetalRoughnessTexture;
		RHI::TextureHandle AoTexture;
		RHI::TextureHandle NormalMapTexture;
	};

	struct Dx12Mesh final
	{
		enum Flags
		{
			kEmpty = 0,
			kContainsNormals = 1 << 0,
			kContainsTexCoords = 1 << 1,
			kContainsTangents = 1 << 2,
		};

		uint32_t Flags;

		std::vector<DirectX::XMFLOAT3> VertexPositions;
		std::vector<DirectX::XMFLOAT2> VertexTexCoords;
		std::vector<DirectX::XMFLOAT3> VertexNormals;
		std::vector<DirectX::XMFLOAT4> VertexTangents;
		std::vector< DirectX::XMFLOAT3> VertexColour;
		std::vector<uint32_t> Indices;

		RHI::BufferHandle VertexGpuBuffer;
		RHI::BufferHandle IndexGpuBuffer;

		// -- TODO: Remove ---
		uint32_t IndexOffset = 0;
		uint32_t VertexOffset = 0;
		// -- TODO: End Remove ---

		uint32_t TotalIndices = 0;
		uint32_t TotalVertices = 0;

		std::vector<SurfaceDesc> SurfaceDesc;
	};

	class Dx12ResourceManager final : public ResourceManager
	{
	public:
		inline static Dx12ResourceManager* Impl = static_cast<Dx12ResourceManager*>(ResourceManager::Ptr);

	public:
		Dx12ResourceManager() = default;

		// -- Create ---
		TextureHandle CreateTexture(RHI::TextureDesc const& desc) override;
		BufferHandle CreateBuffer(RHI::BufferDesc const& desc) override;
		BufferHandle CreateIndexBuffer(RHI::BufferDesc const& desc) override;
		BufferHandle CreateVertexBuffer(RHI::BufferDesc const& desc) override;
		MeshHandle CreateMesh() override;
		MaterialHandle CreateMaterial() override;
		ShaderHandle CreateShader() override;
		LightHandle CreateLight() override;

		// -- Delete ---
		void DeleteTexture(TextureHandle texture) override;
		void DeleteBuffer(BufferHandle buffer) override;
		void DeleteMesh(MeshHandle mesh) override;
		void DeleteShader(ShaderHandle shader) override;
		void DeleteLight(LightHandle light) override;

		// -- Retireve Desc ---
		RHI::TextureDesc GetTextureDesc(TextureHandle texture) override;
		RHI::BufferDesc GetBufferDesc(BufferHandle buffer) override;

		// -- Retrieve Descriptor Indices ---
		RHI::DescriptorIndex GetTextureDescriptorIndex(TextureHandle texture) override;
		RHI::BufferDesc GetBufferDescriptorIndex(BufferHandle buffer) override;


		// -- Material Data ---
		void SetAlbedo(DirectX::XMFLOAT4 const& albedo) override;
		DirectX::XMFLOAT4 GetAlbedo() override;

		void SetEmissive(DirectX::XMFLOAT4 const& emissive) override;
		DirectX::XMFLOAT4 GetEmissive() override;

		void SetMetalness(float metalness) override;
		float GetMetalness() override;

		void SetRoughness(float roughness) override;
		float GetRoughtness() override;

		void SetAO(float ao) override;
		float GetAO() override;
		void SetDoubleSided(bool isDoubleSided) override;
		bool IsDoubleSided() override;

		void SetAlbedoTexture(TextureHandle texture) override;
		TextureHandle GetAlbedoTexture() override;

		void SetMaterialTexture(TextureHandle texture) override;
		TextureHandle GetMaterialTexture() override;

		void SetAoTexture(TextureHandle texture) override;
		TextureHandle GetAoTexture() override;

		void SetNormalMapTexture(TextureHandle texture) override;
		TextureHandle GetNormalMapTexture() override;

		void SetBlendMode(PhxEngine::Renderer::BlendMode blendMode) override;
		PhxEngine::Renderer::BlendMode GetBlendMode() override;
		// -- Material Data END---

	public:
		void ProcessDeleteQueue(uint64_t completedFrame);

	private:
		struct DeleteItem
		{
			uint32_t Frame;
			std::function<void()> DeleteFn;
		};
		std::deque<DeleteItem> m_deleteQueue;

	};
}