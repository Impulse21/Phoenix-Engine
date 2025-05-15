#pragma once

#include <PhxCore/VFS.h>

// -- forward declares ---
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;
struct cgltf_image;
struct cgltf_material;

namespace phxed
{
	class GltfSceneImporter
	{
	public:
		static void Import(const char* filename)
		{
			GltfSceneImporter importer(filename);
			importer.ImportImpl();
		}

	public:
		GltfSceneImporter(const char* filename);
		~GltfSceneImporter();

		void ImportImpl();

	public:
		void ProcessMeshes();
	private:
		phx::FileHandle m_fileHandle;
		std::unordered_map<cgltf_mesh*, std::string> m_meshFilePaths;
		std::unordered_map<cgltf_image*, std::string> m_texturesPaths;
		std::unordered_map< cgltf_material*, std::string> m_materialPaths;
	};

}
