#pragma once

#include <string>
#include <PhxEngine/Core/HashMap.h>
#include "PipelineTypes.h"

struct cgltf_material;
struct cgltf_mesh;
struct cgltf_texture;
struct cgltf_data;

namespace PhxEngine::Core
{
	class IFileSystem;
}

namespace PhxEngine::Pipeline
{
	struct CgltfContext;
	struct ImportedObjects
	{
		PhxEngine::Core::FlexArray<Mesh> Meshes;
		PhxEngine::Core::FlexArray<Material> Materials;
		PhxEngine::Core::FlexArray<Texture> Textures;
	};

	class GltfAssetImporter
	{
	public:
		GltfAssetImporter() = default;

		bool Import(Core::IFileSystem* fileSystem, std::string const& filename, ImportedObjects& importedObjects);

	private:
		bool ImportMesh(cgltf_mesh* gltfMesh, Pipeline::Mesh& outMesh);
		bool ImportMaterial(cgltf_material* gltfMaterial, Pipeline::Material& outMaterial);
		bool ImportTexture(Pipeline::CgltfContext const& cgltfContext, cgltf_texture* cgltfTexture, Pipeline::Texture& outTexture);

	private:
		PhxEngine::Core::unordered_map<cgltf_mesh*, size_t> m_meshIndexLut;
		PhxEngine::Core::unordered_map<cgltf_material*, size_t> m_materialIndexLut;
		PhxEngine::Core::unordered_map<cgltf_texture*, size_t> m_textureIndexLut;
		cgltf_data* m_gltfData;
	};
}

