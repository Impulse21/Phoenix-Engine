#pragma once

#include <PhxEngine/Core/Pool.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Graphics/Enums.h>
#include <DirectXMath.h>


namespace PhxEngine::RHI
{
	struct Texture;
}

namespace PhxEngine::Scene
{
	struct StaticMeshAsset
	{

	};

	struct MaterialAsset
	{
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

		PhxEngine::Graphics::BlendMode BlendMode = Graphics::BlendMode::Opaque;
		DirectX::XMFLOAT4 Albedo = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		float Metalness = 1.0f;
		float Roughness = 1.0f;
		float Ao = 0.4f;
		bool IsDoubleSided = false;

		RHI::TextureHandle AlbedoTexture;
		RHI::TextureHandle MetalRoughnessTexture;
		RHI::TextureHandle AoTexture;
		RHI::TextureHandle NormalMapTexture;

		uint32_t GetRenderTypes();
	};

	using MaterialAssetHandle = Core::Handle<Scene::MaterialAsset>;

	struct TextureAsset
	{
		Core::Handle<RHI::Texture> GpuTexture;

		// Any extra metadata about the texture
	};

	// Make these Ref Counted.
	class AssetStore
	{
	public:
		AssetStore() = default;

		Core::Handle<Scene::StaticMeshAsset> CreateMesh(std::string_view filename, bool isAsync = false);
		Core::Handle<Scene::TextureAsset> CreateTexture(std::string_view filename, bool isAsync = false);
		MaterialAssetHandle CreateMaterial();

		MaterialAsset* GetStaticMeshAsset(Core::Handle<Scene::StaticMeshAsset> handle);
		MaterialAsset* GetMaterialAsset(MaterialAssetHandle handle);

		// Free

	private:
		// Core::Pool<StaticMesh, StaticMesh> m_staticMeshPool;
		// Core::Pool<Material, Material> m_materialPool;
		// Core::Pool<Texture, Texture> m_texturePool;
	};
}