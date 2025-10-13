#include "PhxRenderer/PhxRenderer_pch.h"

#include "ModelResourceHandler.h"
#include "ModelResoure.h"

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceSystem.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::data;
using namespace phx::renderer;

void phx::renderer::ModelResourceHandler::LoadAsync(data::IStreamingManager* streaming_manager, IVirtualFileSystem* vfs, phx::RefCountPtr<phx::Resource> resource, std::string const& virtual_file_path) const
{
	// TODO: Check if cached version is loaded already. If so, load from there.
	RefCountPtr<ModelResoure> model_resource = resource.As<ModelResoure>();
	Result<data::AsyncResourceDescriptor> resource_descriptor = vfs->GetResourceDescriptorForAsync(virtual_file_path);

	if (!resource_descriptor)
	{
		PHX_CORE_ERROR("Failed to find file info '{0}'", virtual_file_path);
		model_resource->state = Resource::State::Error;
		return;
	}

	model_resource->state = Resource::State::Loading;
	ResourceFile::Load(
		streaming_manager,
		resource_descriptor.GetValue(),
		[model_resource](std::shared_ptr<ResourceFile> /*resourceFile*/)
		{
			// auto meshMetadata = reinterpret_cast<const ModelMetadata*>(resourceFile->Metadata->MetadataChunk.Get());
			//resourceFile->metadata->MetadataChunk.Get();

		});
	// TODO: Fix boiler plate stuff.
	std::shared_ptr<char[]> dest = std::make_shared<char[]>(resource_descriptor->length_of_resource);
	data::StreamingRequest request = {
		.operations = {
			{
				.source = {
					.data = resource_descriptor.GetValue(),
					.size = resource_descriptor->length_of_resource,
				},
				.destination = {
					.target = dest,
					.size = resource_descriptor->length_of_resource,
				}
			}
		}
	};

	request.on_complete = [=](data::StreamingResult const& result) mutable {
		if (result.error_code != ErrorCode::Success)
		{
			PHX_CORE_ERROR("Failed to load '{0}'", virtual_file_path);
			model_resource->state = Resource::State::Error;
			return;
		}
	};
	streaming_manager->Submit(std::move(request));
#if false
	ResourceFile::Load(
		resource_system->,
		fileHandle,
		[retVal](std::shared_ptr<ResourceFile> resourceFile)
		{
			auto meshMetadata = reinterpret_cast<const ModelMetadata*>(resourceFile->Metadata->MetadataChunk.Get());
			RequestMeshData(
				retVal,
				resourceFile->AssetStreamer,
				resourceFile->FileHandle,
				meshMetadata,
				resourceFile->Metadata->Chunks);
		});
#endif
}

#if false
void phx::renderer::ModelResourceHandler::RequestMeshData(
	RefCountPtr<ModelResoure> modelResoure,
	std::shared_ptr<IAssetStreamer> const& assetStreamer,
	StreamFileHandle fileHandle,
	const ModelMetadata* meshMetadata,
	const ResourceFileFormat::Chunk* chunks)
{
	// const ResourceFileFormat::Chunk& cpuDataChunk = chunks[0];

	StreamRequest cpuDataRequest; // = StreamRequest::Create(fileHandle, cpuDataChunk.Offset.Offset, cpuDataChunk.UncompressedSize, modelResoure->m_cpuData);
	cpuDataRequest.DebugName = "Mesh CPU Request";

	// TODO: Determine if we should just create one large buffer
	// and alias/srv off it, or create a heap for this resource, 
	// would require an RHI change.
	modelResoure->gemoetry_buffer = RHI::CreateBuffer({
		.DebugName = "Geometry Buffer",
		.Size = meshMetadata->geometry_bufer_size,
		.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::IndexBuffer,
		.MiscFlags = RHI::ResourceMiscFlags::BufferRaw,
		.InitialState = RHI::ResourceStates::Common,
		});

	const ResourceFileFormat::Chunk& gpuDataChunk = chunks[1];
#if true
	StreamRequest gpuDataRequest = {
		.DebugName = "Mesh Geometry Buffer",
		.FileHandle = fileHandle,
		.SrcSize = gpuDataChunk.CompressedSize,
		.DestSize = gpuDataChunk.UncompressedSize,
		.Offset = gpuDataChunk.Offset.Offset,
		.Destination = {.Type = DestinationType::RHI_GpuBuffer, .Buffer = modelResoure->gemoetry_buffer }
	};
#else
	StreamRequest gpuDataRequest = StreamRequest::Create(fileHandle, gpuDataChunk.Offset.Offset, gpuDataChunk.UncompressedSize, modelResoure->m_gpuData);
#endif

	assetStreamer->SubmitBatch({ cpuDataRequest, gpuDataRequest },
		[resource = modelResoure]() {
		});
}
#endif
