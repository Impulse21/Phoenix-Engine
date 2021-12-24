#pragma once

#include <PhxEngine/Renderer/SceneLoader.h>
#include <PhxEngine/Renderer/SceneNodes.h>
#include <PhxEngine/Renderer/TextureCache.h>

#include <PhxEngine/RHI/PhxRHI.h>

#include <unordered_map>
#include <vector>
#include <memory>

class cgltf_data;
class cgltf_material;
class cgltf_mesh;
class cgltf_texture;

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
		GltfSceneLoader(std::shared_ptr<Core::IFileSystem> fs, std::shared_ptr<TextureCache> textureCache)
			: m_fileSystem(fs)
			, m_textureCache(textureCache)
		{}

		Scene* LoadScene(std::string const& fileName);

	private:
		Scene* LoadSceneInternal(
			cgltf_data* gltfData,
			CgltfContext& context);

		std::shared_ptr<RHI::TextureHandle> LoadTexture(
			const cgltf_texture* cglftTexture,
			bool isSRGB,
			const cgltf_data* objects,
			CgltfContext& context);

		void LoadMaterialData(
			const cgltf_material* pMaterials,
			uint32_t materialCount,
			const cgltf_data* objects,
			CgltfContext& context,
			std::unordered_map<const cgltf_material*, std::shared_ptr<Material>>& outMaterials);

		void LoadMeshData(
			const cgltf_mesh* pMeshes,
			uint32_t meshCount,
			std::unordered_map<const cgltf_material*, std::shared_ptr<Material>> const& materialMap,
			std::unordered_map<const cgltf_mesh*, std::shared_ptr< cgltf_mesh>>& outMeshes);

	private:
		std::shared_ptr<Core::IFileSystem> m_fileSystem;
		std::shared_ptr<TextureCache> m_textureCache;
		std::filesystem::path m_filename;
	};
}