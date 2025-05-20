#include "PhxRenderer_pch.h"

#include <PhxResource/ResourceFile.h>

#include "MeshResourceHandler.h"
#include "MeshResource.h"


#include <PhxRhi/RHICore.h>

using namespace phx;
using namespace phx::renderer;


RefCountPtr<IResource> phx::renderer::MeshResourceHandler::LoadFromPak(std::shared_ptr<IAssetStreamer> const& assetStreamer, StreamFileHandle filehandle, PakFileFormat::AssetEntry const& assetEntry) const
{
	auto metadata = reinterpret_cast<const MeshMetadata*>(assetEntry.MetadataChunk.Get());

	auto meshResource = RefCountPtr<MeshResource>::Create(new MeshResource());
	RequestMeshData(meshResource, assetStreamer, filehandle, metadata, assetEntry.Chunks.Get());
	return meshResource;
}

RefCountPtr<IResource> phx::renderer::MeshResourceHandler::LoadLoose(std::shared_ptr<IAssetStreamer> const& assetStreamer, StreamFileHandle fileHandle) const
{
	auto retVal = RefCountPtr<MeshResource>::Create(new MeshResource());

	ResourceFile::Load(
		assetStreamer,
		fileHandle,
		[retVal](std::shared_ptr<ResourceFile> resourceFile)
		{
			auto meshMetadata = reinterpret_cast<const MeshMetadata*>(resourceFile->Metadata->MetadataChunk.Get());
			RequestMeshData(
				retVal,
				resourceFile->AssetStreamer,
				resourceFile->FileHandle,
				meshMetadata,
				resourceFile->Metadata->Chunks);
		});

	return retVal;
}

void phx::renderer::MeshResourceHandler::RequestMeshData(
	RefCountPtr<MeshResource> meshResource,
	std::shared_ptr<IAssetStreamer> const& assetStreamer,
	StreamFileHandle fileHandle,
	const MeshMetadata* meshMetadata,
	const ResourceFileFormat::Chunk* chunks)
{
	const ResourceFileFormat::Chunk& cpuDataChunk = chunks[0];
	
	StreamRequest cpuDataRequest = StreamRequest::Create(fileHandle, cpuDataChunk.Offset.Offset, cpuDataChunk.UncompressedSize, meshResource->m_cpuData); 
	cpuDataRequest.DebugName = "Mesh CPU Request";

	// TODO: Determine if we should just create one large buffer
	// and alias/srv off it, or create a heap for this resource, 
	// would require an RHI change.
	meshResource->m_geometryBuffer = rhi::CreateBuffer({
		.DebugName = "Geometry Buffer",
		.Size = meshMetadata->GeometryBufferSize,
		.BindingFlags = rhi::BindingFlags::ShaderResource | rhi::BindingFlags::IndexBuffer,
		.MiscFlags = rhi::ResourceMiscFlags::BufferRaw,
		.InitialState = rhi::ResourceStates::IndexGpuBuffer | rhi::ResourceStates::ShaderResourceNonPixel | rhi::ResourceStates::CopyDest,
		});
#if true
	const ResourceFileFormat::Chunk& gpuDataChunk = chunks[1];
	StreamRequest gpuDataRequest = {
		.DebugName = "Mesh Geometry Buffer",
		.FileHandle = fileHandle,
		.SrcSize = gpuDataChunk.CompressedSize,
		.DestSize = gpuDataChunk.UncompressedSize,
		.Offset = gpuDataChunk.Offset.Offset,
		.Destination = {.Type = DestinationType::RHI_GpuBuffer, .Buffer = meshResource->m_geometryBuffer }
	};

	assetStreamer->SubmitBatch({ cpuDataRequest, gpuDataRequest },
		[resource = meshResource]() {
				resource->m_status = 0;
		});
#else
	assetStreamer->SubmitBatch({ cpuDataRequest },
		[resource = meshResource]() {
			resource->m_status = 0;
		});
#endif
}
