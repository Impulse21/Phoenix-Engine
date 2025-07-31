#include "ModelImporter_Gltf.h"

#include <PhxCore/Log.h>
#include <PhxCore/SystemTime.h>

#include <memory>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>


namespace
{
	struct CgltfContext
	{
	};

	cgltf_result CgltfReadFile(const cgltf_memory_options*, const cgltf_file_options* /*file_options*/, const char* /*path*/, cgltf_size* /*size*/, void** /*Data*/)
	{
#if false
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<phx::IBlob> dataBlob = context->vfs->ReadFileSynchronous(path).ValueOr(nullptr);
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
#endif
		return cgltf_result_success;
	}

	void CgltfReleaseFile(
		const struct cgltf_memory_options*,
		const struct cgltf_file_options*,
		void*)
	{
		// do nothing
	}
}

phx::Result<ModelData> GltfModelImporter::Import(std::string const& file)
{
	CgltfContext ctx = {};
	cgltf_options options = { };

	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &ctx;

	cgltf_data* raw_gltf_data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, file.c_str(), &raw_gltf_data);

	if (result != cgltf_result_success)
	{
		PHX_ERROR("Couldn't parse glTF file '{0}'", file);
		return phx::make_unexpected(~0ull);
	}

	result = cgltf_load_buffers(&options, raw_gltf_data, file.c_str());
	if (result != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF Binary data '{0}'", result);
		return phx::make_unexpected(~0ull);
	}

	phx::CpuTimer timer;
	// Walk the Scene 
	ModelData data = {};

	return data;
}
