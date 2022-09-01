#pragma once

#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Core/Handle.h>
#include <DirectXMath.h>

// TODO: Replace Descritpors

namespace PhxEngine::Renderer
{
	// -- Enums ---
	enum class BlendMode
	{
		Opaque = 0,
		Alpha,
		Count
	};
	// -- Enums End ---

	struct Buffer;
	using BufferHandle = Core::Handle<PhxEngine::Renderer::Buffer>;

	struct Texture;
	using TextureHandle = Core::Handle<PhxEngine::Renderer::Texture>;

	struct RendererShader;
	using ShaderHandle = Core::Handle<PhxEngine::Renderer::RendererShader>;

	struct Material;
	using MaterialHandle = Core::Handle<PhxEngine::Renderer::Material>;

	struct Light;
	using LightHandle = Core::Handle<PhxEngine::Renderer::Light>;

	struct SurfaceDesc
	{
		MaterialHandle MaterialHandle = {};
		uint32_t IndexOffsetInMesh = 0;
		uint32_t VertexOffsetInMesh = 0;
		uint32_t NumVertices = 0;
		uint32_t NumIndices = 0;

		uint32_t GlobalGeometryIndex = ~0U;
	};

	struct Mesh;
	using MeshHandle = Core::Handle<PhxEngine::Renderer::Mesh>;

	class ResourceManager
	{
	public:
		// Global pointer set by intializer
		inline static ResourceManager* Ptr = nullptr;

	public:
		virtual ~ResourceManager() = default;

		// -- Create ---
		virtual TextureHandle CreateTexture(RHI::TextureDesc const& desc) = 0;
		virtual BufferHandle CreateBuffer(RHI::BufferDesc const& desc) = 0;
		virtual BufferHandle CreateIndexBuffer(RHI::BufferDesc const& desc) = 0;
		virtual BufferHandle CreateVertexBuffer(RHI::BufferDesc const& desc) = 0;
		virtual MeshHandle CreateMesh() = 0;
		virtual MaterialHandle CreateMaterial() = 0;
		virtual ShaderHandle CreateShader() = 0;
		virtual LightHandle CreateLight() = 0;

		// -- Delete ---
		virtual void DeleteTexture(TextureHandle texture) = 0;
		virtual void DeleteBuffer(BufferHandle buffer) = 0;
		virtual void DeleteMesh(MeshHandle mesh) = 0;
		virtual void DeleteShader(ShaderHandle shader) = 0;
		virtual void DeleteLight(LightHandle light) = 0;

		// -- Retireve Desc ---
		virtual RHI::TextureDesc GetTextureDesc(TextureHandle texture) = 0;
		virtual RHI::BufferDesc GetBufferDesc(BufferHandle buffer) = 0;

		// -- Retrieve Descriptor Indices ---
		virtual RHI::DescriptorIndex GetTextureDescriptorIndex(TextureHandle texture) = 0;
		virtual RHI::BufferDesc GetBufferDescriptorIndex(BufferHandle buffer) = 0;


		// -- Material Data ---
		virtual void SetAlbedo(DirectX::XMFLOAT4 const& albedo) = 0;
		virtual DirectX::XMFLOAT4 GetAlbedo() = 0;

		virtual void SetEmissive(DirectX::XMFLOAT4 const& emissive) = 0;
		virtual DirectX::XMFLOAT4 GetEmissive() = 0;

		virtual void SetMetalness(float metalness) = 0;
		virtual float GetMetalness() = 0;

		virtual void SetRoughness(float roughness) = 0;
		virtual float GetRoughtness() = 0;

		virtual void SetAO(float ao) = 0;
		virtual float GetAO() = 0;
		virtual void SetDoubleSided(bool isDoubleSided) = 0;
		virtual bool IsDoubleSided() = 0;

		virtual void SetAlbedoTexture(TextureHandle texture) = 0;
		virtual TextureHandle GetAlbedoTexture() = 0;

		virtual void SetMaterialTexture(TextureHandle texture) = 0;
		virtual TextureHandle GetMaterialTexture() = 0;

		virtual void SetAoTexture(TextureHandle texture) = 0;
		virtual TextureHandle GetAoTexture() = 0;

		virtual void SetNormalMapTexture(TextureHandle texture) = 0;
		virtual TextureHandle GetNormalMapTexture() = 0;

		virtual void SetBlendMode(PhxEngine::Renderer::BlendMode blendMode) = 0;
		virtual PhxEngine::Renderer::BlendMode GetBlendMode() = 0;
		// -- Material Data END ---


		// -- Mesh Data		---
		// -- Mesh Data End ---
	};
}