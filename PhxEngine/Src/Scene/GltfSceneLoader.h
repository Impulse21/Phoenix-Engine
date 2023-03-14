#pragma once

#include "PhxEngine/Scene/SceneLoader.h"
#include <PhxEngine/Scene/Assets.h>
#include <PhxEngine/Scene/Entity.h>

#include "PhxEngine/RHI/PhxRHI.h"
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

	class GltfSceneLoader : public ISceneLoader
	{
	public:
		GltfSceneLoader();

		bool LoadScene(
			std::shared_ptr<Core::IFileSystem> fileSystem,
			std::shared_ptr<Graphics::TextureCache> textureCache,
			std::filesystem::path const& fileName,
			RHI::ICommandList* commandList,
			PhxEngine::Scene::Scene& scene) override;

	private:
		bool LoadSceneInternal(
			cgltf_data* gltfData,
			CgltfContext& context,
			RHI::ICommandList* commandList,
			Scene& scene);

		std::shared_ptr<Assets::Texture> LoadTexture(
			const cgltf_texture* cglftTexture,
			bool isSRGB,
			const cgltf_data* objects,
			CgltfContext& context,
			RHI::ICommandList* commandList);

		void LoadMaterialData(
			const cgltf_material* pMaterials,
			uint32_t materialCount,
			const cgltf_data* objects,
			CgltfContext& context,
			RHI::ICommandList* commandList,
			Scene& scene);

		void LoadMeshData(
			const cgltf_mesh* pMeshes,
			uint32_t meshCount,
			ICommandList* commandList,
			Scene& scene);

		void LoadNode(
			const cgltf_node& gltfNode,
			PhxEngine::Scene::Entity parent,
			Scene& scene);

	private:
		std::filesystem::path m_filename;

		std::shared_ptr<Graphics::TextureCache> m_textureCache;

		Entity m_rootNode;
		std::unordered_map<const cgltf_material*, Entity> m_materialEntityMap;
		std::unordered_map<const cgltf_mesh*, std::vector<Entity>> m_meshEntityMap;
	};
}