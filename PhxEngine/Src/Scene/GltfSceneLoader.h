#pragma once

#include "SceneLoader.h"
#include "Systems/Ecs.h"

#include "Graphics/RHI/PhxRHI.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>

struct cgltf_data;
struct cgltf_material;
struct cgltf_mesh;
struct cgltf_texture;
struct cgltf_camera;
struct cgltf_node;

struct CgltfContext;

namespace PhxEngine::Graphics
{
	class TextureCache;
}

namespace PhxEngine::Scene
{
	static std::unordered_map<std::string, bool> sSupportedExtensions;

	class GltfSceneLoader : public ISceneLoader
	{
	public:
		GltfSceneLoader(RHI::IGraphicsDevice* graphcisDevice, std::shared_ptr<Graphics::TextureCache> textureCache)
			: m_graphicsDevice(graphcisDevice)
			, m_textureCache(textureCache)
		{}

		bool LoadScene(std::string const& fileName, RHI::CommandListHandle commandList, Scene& scene) override;

	private:
		bool LoadSceneInternal(
			cgltf_data* gltfData,
			CgltfContext& context,
			RHI::CommandListHandle commandList,
			Scene& scene);

		RHI::TextureHandle LoadTexture(
			const cgltf_texture* cglftTexture,
			bool isSRGB,
			const cgltf_data* objects,
			CgltfContext& context,
			RHI::CommandListHandle commandList);

		void LoadMaterialData(
			const cgltf_material* pMaterials,
			uint32_t materialCount,
			const cgltf_data* objects,
			CgltfContext& context,
			RHI::CommandListHandle commandList,
			Scene& scene);

		void LoadMeshData(
			const cgltf_mesh* pMeshes,
			uint32_t meshCount,
			Scene& scene);

		void LoadNode(
			const cgltf_node& gltfNode,
			ECS::Entity parent,
			Scene& scene);

	private:
		RHI::IGraphicsDevice* m_graphicsDevice;
		std::shared_ptr<Graphics::TextureCache> m_textureCache;

		std::filesystem::path m_filename;

		// LUT helpers
		std::unordered_map<const cgltf_material*, ECS::Entity> m_materialMap;
		std::unordered_map<const cgltf_mesh*, ECS::Entity> m_meshMap;
	};
}