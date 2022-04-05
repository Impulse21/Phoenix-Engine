#pragma once

#include <PhxEngine/Renderer/SceneLoader.h>
#include <PhxEngine/Renderer/SceneNodes.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/Ecs/Ecs.h>

#include <PhxEngine/RHI/PhxRHI.h>

#include <unordered_map>
#include <vector>
#include <memory>

struct cgltf_data;
struct cgltf_material;
struct cgltf_mesh;
struct cgltf_texture;
struct cgltf_camera;
struct cgltf_node;

struct CgltfContext;

namespace PhxEngine::Core
{
	class IFileSystem;
}

namespace PhxEngine::Renderer
{
	static std::unordered_map<std::string, bool> sSupportedExtensions;

	class GltfSceneLoader : public ISceneLoader
	{
	public:
		GltfSceneLoader(std::shared_ptr<Core::IFileSystem> fs, std::shared_ptr<TextureCache> textureCache, RHI::IGraphicsDevice* graphcisDevice)
			: m_fileSystem(fs)
			, m_textureCache(textureCache)
			, m_graphicsDevice(graphcisDevice)
		{}

		bool LoadScene(std::string const& fileName, RHI::CommandListHandle commandList, New::Scene& scene) override;

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
			ECS::Entity parent,
			New::Scene& scene);

	private:
		RHI::IGraphicsDevice* m_graphicsDevice;
		std::shared_ptr<Core::IFileSystem> m_fileSystem;
		std::shared_ptr<TextureCache> m_textureCache;
		std::filesystem::path m_filename;

		// LUT helpers
		std::unordered_map<const cgltf_material*, ECS::Entity> m_materialMap;
		std::unordered_map<const cgltf_mesh*, ECS::Entity> m_meshMap;
	};
}