#include "phxModelImporterGltf.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <Core/phxVirtualFileSystem.h>
#include <Core/phxLog.h>

#include <Renderer/phxConstantBuffers.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
	cgltf_result CgltfReadFile(const cgltf_memory_options* memory_options, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<IBlob> dataBlob = context->FileSystem->ReadFile(path);
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
}

bool phx::phxModelImporterGltf::Import(IFileSystem* fs, std::string const& filename, ModelData& outModel)
{
	// Load GLF File into memory
	this->m_cgltfContext =
	{
		.FileSystem = fs,
		.Blobs = {}
	};

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &this->m_cgltfContext;


	std::unique_ptr<IBlob> blob = fs->ReadFile(filename);
	if (!blob)
	{
		PHX_ERROR("Couldn't Read file %s", filename.c_str());
		return false;
	}

	this->m_gltfData = nullptr;
	cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &this->m_gltfData);
	if (res != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF file %s", filename.c_str());
		return false;
	}

	res = cgltf_load_buffers(&options, this->m_gltfData, filename.c_str());
	if (res != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF Binary data %s", filename.c_str());
		return false;
	}

	this->BuildMaterials(outModel);

    return true;
}

void phx::phxModelImporterGltf::BuildMaterials(ModelData& outModel)
{
	static_assert((_alignof(MaterialConstants) & 255) == 0, "CBVs need 256 byte alignment");

	// Replace texture filename extensions with "DDS" in the string table
	outModel.TextureNames.resize(this->m_gltfData->textures_count);
	for (size_t i = 0; i < this->m_gltfData->textures_count; ++i)
	{
		outModel.TextureNames[i] = this->m_gltfData->textures[i].name;
	}

	const uint32_t numMaterials = (uint32_t)this->m_gltfData->materials_count;

	outModel.MaterialConstants.resize(numMaterials);
	outModel.MaterialTextures.resize(numMaterials);
}
