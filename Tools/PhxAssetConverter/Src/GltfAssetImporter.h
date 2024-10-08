#pragma once

#include <string>
#include "PipelineTypes.h"
#include <unordered_map>
#include <vector>

struct cgltf_material;
struct cgltf_mesh;
struct cgltf_texture;
struct cgltf_data;

namespace PhxEngine
{
	class IFileSystem;
}

namespace PhxEngine::Pipeline
{
	struct CgltfContext;
	struct ImportedObjects
	{
		std::vector<Mesh> Meshes;
		std::vector<Material> Materials;
		std::vector<Texture> Textures;
	};

	class GltfAssetImporter
	{
	public:
		GltfAssetImporter() = default;

		bool Import(IFileSystem* fileSystem, std::string const& filename, ImportedObjects& importedObjects);

	private:
		bool ImportMesh(cgltf_mesh* gltfMesh, Pipeline::Mesh& outMesh);
		bool ImportMaterial(cgltf_material* gltfMaterial, Pipeline::Material& outMaterial);
		bool ImportTexture(Pipeline::CgltfContext const& cgltfContext, cgltf_texture* cgltfTexture, Pipeline::Texture& outTexture);

	private:
		std::unordered_map<cgltf_mesh*, size_t> m_meshIndexLut;
		std::unordered_map<cgltf_material*, size_t> m_materialIndexLut;
		std::unordered_map<cgltf_texture*, size_t> m_textureIndexLut;
		cgltf_data* m_gltfData;
	};
}

