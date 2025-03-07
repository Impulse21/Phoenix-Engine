#include "PhxRenderer_pch.h"

#include "MeshResourceHandler.h"
#include "MeshResource.h"

#include <PhxResource/ResourceFileFormat.h>
#include <PhxRhi/RHICore.h>

using namespace phx;
using namespace phx::renderer;

RefCountPtr<IResource> phx::renderer::MeshResourceHandler::Load(std::shared_ptr<IAssetStreamer> const& assetStreamer, StreamFileHandle filehandle, PakFileFormat::AssetEntry const& assetEntry) const
{
	auto metadata = reinterpret_cast<const MeshMetadata*>(assetEntry.MetadataChunk.Get());

	std::unique_ptr<MeshResource> meshResource = std::make_unique<MeshResource>();

	const ResourceFileFormat::Chunk* cpuDataChunk = assetEntry.Chunks.Get();
	StreamRequest cpuDataRequest = {
		.FileHandle = filehandle,
		.Offset = cpuDataChunk->Offset.Offset,
		.Size = cpuDataChunk->UncompressedSize,
		.Destination = &meshResource->m_cpuData
	};

	// TODO: Determine if we should just create one large buffer
	// and alias/srv off it, or create a heap for this resource, 
	// would require an RHI change.
	meshResource->m_geometryBuffer = rhi::CreateBuffer({
		.DebugName = "Geometry Buffer",
		.SizeInBytes = metadata->GeometryBufferSize,
		.MiscFlags = rhi::ResourceMiscFlags::BufferRaw,
		.InitialState = rhi::ResourceStates::Common
	});

	StreamRequest gpuDataRequest = {
		.FileHandle = filehandle,
		.Offset = cpuDataChunk->Offset.Offset,
		.Size = cpuDataChunk->UncompressedSize,
	};

	assetStreamer->SubmitBatch({ cpuDataRequest, gpuDataRequest },
		[resource = meshResource.get()]() {
			resource->m_status = 0;
		});
	
	return RefCountPtr<IResource>::Create(meshResource.release());
}
