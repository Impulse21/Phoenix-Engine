#pragma once

#include "phxModelImporter.h"
#include <unordered_map>
#include <vector>
#include <memory>

struct cgltf_material;
struct cgltf_mesh;
struct cgltf_texture;
struct cgltf_data;

namespace phx
{
	struct CgltfContext
	{
		IFileSystem* FileSystem;
		std::vector<std::shared_ptr<IBlob>> Blobs;
	};

	class phxModelImporterGltf final : public ModelImporter
	{
	public:
		phxModelImporterGltf() = default;
		~phxModelImporterGltf() override = default;

		bool Import(IFileSystem* fs, std::string const& filename, ModelData& outModel) override;

	private:
		void BuildMaterials(ModelData& outModel);

	private:
		std::unordered_map<cgltf_mesh*, size_t> m_meshIndexLut;
		std::unordered_map<cgltf_material*, size_t> m_materialIndexLut;
		std::unordered_map<cgltf_texture*, size_t> m_textureIndexLut;
		cgltf_data* m_gltfData;
		CgltfContext m_cgltfContext;
	};
}

