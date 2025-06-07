#include "GltfFileHandler.h"

#include <PhxEngine/JobSystem.h>

#include <PhxWorld/SceneBlueprint.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxData/IVirtualFileSystem.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phx;
using namespace phxed;

namespace
{
	struct CgltfContext
	{
		phx::data::IVirtualFileSystem* FileSystem;
		std::vector<std::shared_ptr<phx::IBlob>> Blobs;
	};
#if false
	cgltf_result CgltfReadFile(const cgltf_memory_options*, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<phx::IBlob> dataBlob = context->FileSystem->ReadFileSynchronous(path).ValueOr(nullptr);
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

	void CgltfReleaseFile(
		const struct cgltf_memory_options*,
		const struct cgltf_file_options*,
		void*)
	{
		// do nothing
	}
#endif
}
phx::RefCountPtr<phx::Resource> phxed::GltfFileHandler::LoadFromPak(data::IVirtualFileSystem* /*fs*/) const
{
    throw std::runtime_error("Not implemented yet");
    return nullptr;
}

phx::RefCountPtr<phx::Resource> phxed::GltfFileHandler::LoadLoose(data::IVirtualFileSystem* /*fs*/) const
{
	auto sceneBlueprint = phx::RefCountPtr<SceneBlueprint>::Create(new SceneBlueprint);

	// TODO implement a DirectStorage style interface for streaming asset.
	JobSystem::SubmitJob([=](JobContext const&) {
#if false
			const char* gltfFilename = fs->GetFilename(handle);
			// Load GLF File into memory
			CgltfContext context =
			{
				.FileSystem = fs,
				.Blobs = {}
			};

			cgltf_options options = { };
			options.file.read = &CgltfReadFile;
			options.file.release = &CgltfReleaseFile;
			options.file.user_data = &context;

			std::unique_ptr<phx::IBlob> blob = fs->ReadFile(handle);
			if (!blob)
			{
				PHX_CORE_ERROR("Couldn't Read file gltf file '{0}'", gltfFilename);
				sceneBlueprint->State = Resource::State::Error;
				return;
			}

			cgltf_data* gltfData = nullptr;
			cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &gltfData);
			if (res != cgltf_result_success)
			{
				PHX_ERROR("Couldn't load glTF file '{0}'", gltfFilename);
				sceneBlueprint->State = Resource::State::Error;
				return;
			}

			res = cgltf_load_buffers(&options, gltfData, gltfFilename);
			if (res != cgltf_result_success)
			{
				PHX_ERROR("Couldn't load glTF Binary data '{0}'", gltfFilename);

				sceneBlueprint->State = Resource::State::Error;
				return;
			}

			sceneBlueprint->State = Resource::State::Loaded;
#endif
		},
		JobSystem::Type::Streaming);
	

    return sceneBlueprint;
}
