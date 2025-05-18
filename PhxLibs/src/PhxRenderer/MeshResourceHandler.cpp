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

	std::unique_ptr<MeshResource> meshResource = std::make_unique<MeshResource>();

	const ResourceFileFormat::Chunk* cpuDataChunk = assetEntry.Chunks.Get();
	StreamRequest cpuDataRequest = {
		.DebugName = "Mesh CPU Request",
		.FileHandle = filehandle,
		.SrcSize = cpuDataChunk->UncompressedSize,
		.DestSize = cpuDataChunk->UncompressedSize,
		.Offset = cpuDataChunk->Offset.Offset,
		.Destination = { .Memory = &meshResource->m_cpuData }
	};

	// TODO: Determine if we should just create one large buffer
	// and alias/srv off it, or create a heap for this resource, 
	// would require an RHI change.
	meshResource->m_geometryBuffer = rhi::CreateBuffer({
		.DebugName = "Geometry Buffer",
		.Size = metadata->GeometryBufferSize,
		.BindingFlags = rhi::BindingFlags::ShaderResource | rhi::BindingFlags::IndexBuffer,
		.MiscFlags = rhi::ResourceMiscFlags::BufferRaw,
		.InitialState = rhi::ResourceStates::IndexGpuBuffer | rhi::ResourceStates::ShaderResourceNonPixel,
	});

	StreamRequest gpuDataRequest = {
		.DebugName = "Mesh Geometry Buffer",
		.FileHandle = filehandle,
		.SrcSize = cpuDataChunk->CompressedSize,
		.DestSize = cpuDataChunk->UncompressedSize,
		.Offset = cpuDataChunk->Offset.Offset,
		.Destination = { .Type = DestinationType::RHI_GpuBuffer, .Buffer = meshResource->m_geometryBuffer }
	};

	assetStreamer->SubmitBatch({ cpuDataRequest, gpuDataRequest },
		[resource = meshResource.get()]() {
			resource->m_status = 0;
		});
	
	return RefCountPtr<IResource>::Create(meshResource.release());
}

RefCountPtr<IResource> phx::renderer::MeshResourceHandler::LoadLoose(std::shared_ptr<IAssetStreamer> const& assetStreamer, StreamFileHandle fileHandle) const
{
	std::unique_ptr<MeshResource> meshResource = std::make_unique<MeshResource>();

	std::shared_ptr<phx::ResourceFile> resourceFile = std::make_shared<phx::ResourceFile>();

	assetStreamer->Submit({
			.DebugName = "Resource Header Load",
			.FileHandle = fileHandle,
			.SrcSize = sizeof(PakFileFormat::Header),
			.DestSize = sizeof(PakFileFormat::Header),
			.Destination = {.Memory = &resourceFile->Header }
		},
		[resourceFile] 
		{
			// Once header is loaded, load metadata
		});

	return RefCountPtr<IResource>::Create(meshResource.release());
}
