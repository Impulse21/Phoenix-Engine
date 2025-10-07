#include "PhxRenderer/PhxRenderer_pch.h"

#include "ModelResourceHandler.h"
#include "ModelResoure.h"

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceSystem.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;

void phx::renderer::ModelResourceHandler::LoadAsync(data::IStreamingManager* /*streaming_manager*/, data::IVirtualFileSystem* /*vfs*/, RefCountPtr<Resource> /*resource*/, std::string const& /*virtual_file_path*/) const
{
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
