#include "AssetImporter_Gltf.h"

#include <PhxEngine/JobSystem.h>

#include <PhxWorld/WorldMetadata.def.h>
#include <PhxResource/ResourceSystem.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IAsyncIOSystem.h>
#include <PhxCore/Math.h>

#include <PhxRenderer/MeshResource.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phx;
using namespace phxed;

namespace
{
	cgltf_result CgltfReadFile(const cgltf_memory_options*, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		// TODO: This should load through loading system
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

phx::RefCountPtr<phx::data::Asset> GltfFileImporter::ImportAsync(phx::data::IVirtualFileSystem* vfs, phx::data::IAsyncIOSystem* loader,  const char* virtual_file_path) const
{
	CgltfContext ctx = {};
	ctx.scene_resource = phx::RefCountPtr<SceneBlueprint>::Create(new SceneBlueprint);
	ctx.resource_descriptor = vfs->GetResourceDescriptorForAsync(virtual_file_path);
	if (!ctx.resource_descriptor)
	{
		PHX_CORE_ERROR("[GLTF Handler] Failed to find file info '{0}'", virtual_file_path);
		ctx.scene_resource->State = Resource::State::Error;
		return ctx.scene_resource;
	}

	ctx.scene_resource->State = Resource::State::Loading;
	ctx.virtual_file_path = virtual_file_path;
	ctx.vfs = vfs;
	ctx.loader = loader;

	data::AsyncReadRequest request = {
		.resource_descriptor = ctx.resource_descriptor.GetValue(),
		.bytes_to_read = ctx.resource_descriptor->length_of_resource,
	};

	request.callback = [ctx](data::AsyncReadResult const& result) mutable {
		OnMainFileLoaded(result, ctx);
	};

	loader->QueueRead(std::move(request));
	

    return ctx.scene_resource;
}

void phxed::GltfFileImporter::OnMainFileLoaded(phx::data::AsyncReadResult const& result, CgltfContext& ctx)
{
	if (!result.success)
	{
		PHX_CORE_ERROR("[GLTF Handler] Failed read file '{0}' -> {1}", ctx.virtual_file_path, result.error_message);
		ctx.scene_resource->State = Resource::State::Error;
		return;
	}

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &ctx;

	cgltf_data* raw_gltf_data = nullptr;
	cgltf_result res = cgltf_parse(
		&options,
		result.data_buffer.data(),
		result.bytes_actually_read,
		&raw_gltf_data);

	if (res != cgltf_result_success)
	{
		PHX_ERROR("Couldn't parse glTF file '{0}'", ctx.virtual_file_path);
		ctx.scene_resource->State = Resource::State::Error;
		return;
	}

	const char* gltf_filename = ctx.resource_descriptor->os_path_or_pak_path.c_str();
	res = cgltf_load_buffers(&options, raw_gltf_data, gltf_filename);
	if (res != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF Binary data '{0}'", gltf_filename);

		ctx.scene_resource->State = Resource::State::Error;
		return;
	}

	// Load Node Data
	SceneNode rootNode = {
		.name = ctx.virtual_file_path,
	};

	NodeHandle rootHandle = ctx.scene_resource->AddNode(std::move(rootNode));
	JobSystem::Barrier sub_resource_barrier;
	ctx.sub_resource_barrier = &sub_resource_barrier;

	cgltf_scene* gltfScene = raw_gltf_data->scene;
	for (size_t i = 0; i < gltfScene->nodes_count; i++)
	{
		// Load Node Data
		LoadNodeRec(ctx, *gltfScene->nodes[i], *ctx.scene_resource, rootHandle);
	}

	JobSystem::Wait(sub_resource_barrier);

	cgltf_free(raw_gltf_data);

	ctx.scene_resource->State = Resource::State::Loaded;
}

void phxed::GltfFileImporter::LoadNodeRec(CgltfContext& /*ctx*/, cgltf_node const& /*gltfNode*/, SceneBlueprint& /*scene*/, NodeHandle /*parent*/)
{
}
