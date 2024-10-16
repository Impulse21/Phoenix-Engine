#pragma once

#include <PhxEngine/Engine/World.h>
#include <future>

#include <functional>
namespace PhxEngine::Editor
{
	class GltfModelLoader
	{
	public:
		std::future<bool> LoadModelAsync(std::string const& filename, Core::IFileSystem* fileSystem, World& world);

		void SetProgressCallback(std::function<void(std::string_view, uint32_t, uint32_t)>& func) { this->m_progressCallback = func; }
		struct CgltfContext
		{
			Core::IFileSystem* FileSystem;
			std::vector<std::shared_ptr<Core::IBlob>> Blobs;
		};

	private:
#if 0 
		void LoadMaterials(
			Core::Span<cgltf_material> cgltfMateirals,
			const cgltf_data* objects,
			CgltfContext& context,
			World& outWorld);

		void LoadMeshs(
			Core::Span<cgltf_mesh> cgltfMeshes,
			World& outWorld);

		void LoadNodeRec(
			const cgltf_node& gltfNode,
			std::shared_ptr<Node> parent,
			World& outWorld);

	private:
		static cgltf_result CgltfReadFile(
			const struct cgltf_memory_options* memory_options,
			const struct cgltf_file_options* file_options,
			const char* path,
			cgltf_size* size,
			void** Data);

		static void CgltfReleaseFile(
			const struct cgltf_memory_options*,
			const struct cgltf_file_options*,
			void*)
		{
			// do nothing
		}
#endif
	private:
		std::string m_filename;
		std::function<void(std::string_view, uint32_t, uint32_t)> m_progressCallback;
	};
}


