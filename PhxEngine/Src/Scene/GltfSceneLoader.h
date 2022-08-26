#pragma once

#include "PhxEngine/Scene/SceneLoader.h"
#include "PhxEngine/Systems/Ecs.h"
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

	class GltfSceneLoader : public ISceneLoader
	{
	public:
		GltfSceneLoader(RHI::IGraphicsDevice* graphcisDevice, std::shared_ptr<Graphics::TextureCache> textureCache)
			: m_graphicsDevice(graphcisDevice)
			, m_textureCache(textureCache)
		{}

		bool LoadScene(
			std::string const& fileName,
			RHI::CommandListHandle commandList, 
			Legacy::Scene& scene) override;

	private:
		bool LoadSceneInternal(
			cgltf_data* gltfData,
			CgltfContext& context,
			RHI::CommandListHandle commandList,
			Legacy::Scene& scene);

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
			Legacy::Scene& scene);

		void LoadMeshData(
			const cgltf_mesh* pMeshes,
			uint32_t meshCount,
			Legacy::Scene& scene);

		void LoadNode(
			const cgltf_node& gltfNode,
			ECS::Entity parent,
			Legacy::Scene& scene);

	private:
		RHI::IGraphicsDevice* m_graphicsDevice;
		std::shared_ptr<Graphics::TextureCache> m_textureCache;

		std::filesystem::path m_filename;

		// LUT helpers
		std::unordered_map<const cgltf_material*, ECS::Entity> m_materialMap;
		std::unordered_map<const cgltf_mesh*, ECS::Entity> m_meshMap;
	};

	namespace New
	{
		class GltfSceneLoader : public New::ISceneLoader
		{
		public:
			GltfSceneLoader() = default;

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

			// LUT helpers
			// Cache Handles TODO
			std::unordered_map<const cgltf_material*, PhxEngine::Scene::MaterialAssetHandle> m_materialMap;
			std::unordered_map<const cgltf_mesh*, ECS::Entity> m_meshMap;
		};
	}
}