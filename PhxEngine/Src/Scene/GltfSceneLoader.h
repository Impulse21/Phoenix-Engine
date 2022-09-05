#pragma once

#include "PhxEngine/Scene/SceneLoader.h"
#include "PhxEngine/Systems/Ecs.h"
#include <PhxEngine/Scene/Assets.h>
#include <PhxEngine/Scene/Entity.h>

#include "PhxEngine/Graphics/RHI/PhxRHI.h"
#include <PhxEngine/Scene/AssetStore.h>

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

	class GltfSceneLoader : public New::ISceneLoader
	{
	public:
		GltfSceneLoader();

		bool LoadScene(
			std::string const& fileName,
			RHI::CommandListHandle commandList,
			New::Scene& scene) override;

	private:
		bool LoadSceneInternal(
			cgltf_data* gltfData,
			CgltfContext& context,
			RHI::CommandListHandle commandList,
			New::Scene& scene);

		std::shared_ptr<Assets::Texture> LoadTexture(
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
			New::Scene& scene);

		void LoadMeshData(
			const cgltf_mesh* pMeshes,
			uint32_t meshCount,
			New::Scene& scene);

		void LoadNode(
			const cgltf_node& gltfNode,
			PhxEngine::Scene::Entity parent,
			New::Scene& scene);

	private:
		std::filesystem::path m_filename;

		std::unique_ptr<Graphics::TextureCache> m_textureCache;

		// LUT helpers
		// Cache Handles TODO
		std::unordered_map<const cgltf_material*, std::shared_ptr<Assets::StandardMaterial>> m_materialMap;
		std::unordered_map<const cgltf_mesh*, std::shared_ptr<Assets::Mesh>> m_meshMap;
	};
}