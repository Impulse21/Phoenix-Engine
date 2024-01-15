#include "GltfModelLoader.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <taskflow/taskflow.hpp>
#include <PhxEngine/Engine/EngineRoot.h>

using namespace PhxEngine;
using namespace PhxEngine::Editor;

std::future<bool> GltfModelLoader::LoadModelAsync(std::string const& filename, Core::IFileSystem* fileSystem, World::World& world)
{
	return Engine::GetTaskExecutor().async([&]() {
#if 0 
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

			// Assert Wait for textures to load
			this->LoadMaterials(
				Core::Span<cgltf_material>(objects->materials, objects->materials_count),
				objects,
				cgltfContext,
				world);

			this->LoadMeshs(
				Core::Span<cgltf_mesh>(objects->meshes, objects->meshes_count),
				world);

			std::shared_ptr<Node> rootNode = std::make_shared<Node>();
			world.AttachRoot(rootNode);
			// Load Node Data
			for (size_t i = 0; i < objects->scene->nodes_count; i++)
			{
				// Load Node Data
				this->LoadNodeRec(*objects->scene->nodes[i], rootNode, world);
			}
#endif
			return true;
		});
}

#if 0

void PhxEngine::Editor::GltfModelLoader::LoadMaterials(Core::Span<cgltf_material> cgltfMateirals, const cgltf_data* objects, CgltfContext& context, World& outWorld)
{
#if false
	for (int i = 0; i < cgltfMateirals.Size(); i++)
	{
		const auto& cgltfMtl = cgltfMateirals[i];

		std::string name = cgltfMtl.name ? cgltfMtl.name : "Material " + std::to_string(i);
		Assets::MaterialHandle material = Assets::AssetStore::RetrieveOrCreateMaterial(name.c_str());

		if (cgltfMtl.alpha_mode != cgltf_alpha_mode_opaque)
		{
			mtl.BlendMode = Renderer::BlendMode::Alpha;
		}

		if (cgltfMtl.has_pbr_specular_glossiness)
		{
			// // LOG_CORE_WARN("Material %s contains unsupported extension 'PBR Specular_Glossiness' workflow ");
		}
		else if (cgltfMtl.has_pbr_metallic_roughness)
		{
			mtl.BaseColourTexture = this->LoadTexture(
				cgltfMtl.pbr_metallic_roughness.base_color_texture.texture,
				true,
				objects,
				context,
				commandList);

			mtl.BaseColour =
			{
				cgltfMtl.pbr_metallic_roughness.base_color_factor[0],
				cgltfMtl.pbr_metallic_roughness.base_color_factor[1],
				cgltfMtl.pbr_metallic_roughness.base_color_factor[2],
				cgltfMtl.pbr_metallic_roughness.base_color_factor[3]
			};

			mtl.MetalRoughnessTexture = this->LoadTexture(
				cgltfMtl.pbr_metallic_roughness.metallic_roughness_texture.texture,
				false,
				objects,
				context,
				commandList);

			mtl.Metalness = cgltfMtl.pbr_metallic_roughness.metallic_factor;
			mtl.Roughness = cgltfMtl.pbr_metallic_roughness.roughness_factor;
		}

		// Load Normal map
		mtl.NormalMapTexture = this->LoadTexture(
			cgltfMtl.normal_texture.texture,
			false,
			objects,
			context,
			commandList);

		// TODO: Emmisive
		mtl.IsDoubleSided = cgltfMtl.double_sided;
#endif
}

cgltf_result PhxEngine::Editor::GltfModelLoader::CgltfReadFile(const cgltf_memory_options* memory_options, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
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
#endif