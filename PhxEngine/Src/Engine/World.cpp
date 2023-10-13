#include <PhxEngine/Engine/World.h>

#include <filesystem>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
using namespace PhxEngine;

namespace 
{
	class GltfWorldLoader final: public IWorldLoader
	{
	public:
		bool LoadWorld(std::string const& filename, Core::IFileSystem* fileSystem, World& outWorld)
		{
			this->m_filename = std::filesystem::path(filename).lexically_normal().generic_string();
			CgltfContext cgltfContext =
			{
				.FileSystem = fileSystem,
				.Blobs = {}
			};

			cgltf_options options = { };
			options.file.read = &CgltfReadFile;
			options.file.release = &CgltfReleaseFile;
			options.file.user_data = &cgltfContext;

			std::unique_ptr<Core::IBlob> blob = fileSystem->ReadFile(this->m_filename);
			if (!blob)
			{
				return false;
			}

			cgltf_data* objects = nullptr;
			cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &objects);
			if (res != cgltf_result_success)
			{
				// LOG_CORE_ERROR("Coouldn't load glTF file %s", normalizedFileName.c_str());
				return false;
			}

			res = cgltf_load_buffers(&options, objects, this->m_filename.c_str());
			if (res != cgltf_result_success)
			{
				// LOG_CORE_ERROR("Failed to load buffers for glTF file '%s'", normalizedFileName.c_str());
				return false;
			}

			this->LoadMaterials(
				Core::Span<cgltf_material>(objects->materials, objects->materials_count),
				objects,
				cgltfContext,
				outWorld);

			this->LoadMeshs(
				Core::Span<cgltf_mesh>(objects->meshes, objects->meshes_count),
				outWorld);

			std::string rootName = std::filesystem::path(this->m_filename).stem().generic_string();
			
			// Create Root Nodes

			std::shared_ptr<Node> rootNode = std::make_shared<Node>();
			outWorld.AttachRoot(rootNode);

			// Load Node Data
			for (size_t i = 0; i < objects->scene->nodes_count; i++)
			{
				// Load Node Data
				this->LoadNodeRec(*objects->scene->nodes[i], rootNode, outWorld);
			}

			return true;
		};

	private:
		struct CgltfContext
		{
			Core::IFileSystem* FileSystem;
			std::vector<std::shared_ptr<Core::IBlob>> Blobs;
		};
	private:
		void LoadMaterials(
			Core::Span<cgltf_material> cgltfMateirals,
			const cgltf_data* objects,
			CgltfContext& context,
			World& outWorld)
		{

		}

		void LoadMeshs(
			Core::Span<cgltf_mesh> cgltfMeshes,
			World& outWorld)
		{

		}

		void LoadNodeRec(
			const cgltf_node& gltfNode,
			std::shared_ptr<Node> parent,
			World& outWorld)
		{

		}

	private:
		static cgltf_result CgltfReadFile(
			const struct cgltf_memory_options* memory_options,
			const struct cgltf_file_options* file_options,
			const char* path,
			cgltf_size* size,
			void** Data)
		{
			CgltfContext* context = (CgltfContext*)file_options->user_data;

			std::unique_ptr<Core::IBlob> dataBlob = context->FileSystem->ReadFile(path);
			if (!dataBlob)
			{
				return cgltf_result_file_not_found;
			}

			if (size)
			{
				*size = dataBlob->Size();
			}

			if (Data)
			{
				*Data = (void*)dataBlob->Data();  // NOLINT(clang-diagnostic-cast-qual)
			}

			context->Blobs.push_back(std::move(dataBlob));

			return cgltf_result_success;
		}

		static void CgltfReleaseFile(
				const struct cgltf_memory_options*,
				const struct cgltf_file_options*,
				void*)
		{
			// do nothing
		}
	private:
		std::string m_filename;
	};
}

std::unique_ptr<IWorldLoader> PhxEngine::WorldLoaderFactory::CreateGltfWorldLoader()
{
	return std::make_unique<GltfWorldLoader>();
}

void PhxEngine::World::AttachRoot(std::shared_ptr<Node> root)
{
}
