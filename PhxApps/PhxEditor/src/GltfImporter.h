#pragma once

#include <PhxCore/VFS.h>

// -- forward declares ---
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;
struct cgltf_image;
struct cgltf_material;

namespace phx
{
	class IFileSystem;
	class IResource;
	class World;
}
namespace phxed
{
	class GltfSceneImporter
	{
	public:
		static bool Import(phx::IFileSystem* fs, const char* filename, phx::World& outWorld)
		{
			GltfSceneImporter importer(fs, filename, outWorld);
			return importer.ImportImpl();
		}

	public:
		GltfSceneImporter(phx::IFileSystem* fs, const char* filename, phx::World& outWorld)
			: m_fs(fs)
			, m_filename(filename)
			, m_out(outWorld)
		{

		}
		~GltfSceneImporter();

		bool ImportImpl();

	public:

		std::shared_ptr<IResource> LoadTexture_Threaded();

		void LoadMaterialData_Threaded();

		void LoadMeshData_Thread();

	private:
		const char* m_filename;
		phx::IFileSystem* m_fs;
		phx::World& m_out;
		cgltf_data* m_gltfData;

		std::unordered_map<cgltf_mesh*, std::string> m_meshFilePaths;
		std::unordered_map<cgltf_image*, std::string> m_texturesPaths;
		std::unordered_map< cgltf_material*, std::string> m_materialPaths;
	};

}
