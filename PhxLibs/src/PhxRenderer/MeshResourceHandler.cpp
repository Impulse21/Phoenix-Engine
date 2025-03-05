#include "PhxRenderer_pch.h"

#include "MeshResourceHandler.h"
#include "MeshResource.h"

#include <PhxResource/ChunkFileFormat.h>

using namespace phx;
using namespace phx::renderer;

RefCountPtr<IResource> phx::renderer::MeshResourceHandler::Load(std::shared_ptr<IAssetStreamer> const& /*assetStreamer*/, StreamFileHandle /*filehandle*/, PakFileFormat::AssetEntry const& assetEntry) const
{
	const MeshCpuMetadata* meshCpuData = reinterpret_cast<const MeshCpuMetadata*>(assetEntry.MetadataChunk.Get());

	assert(meshCpuData->NumDraws >= 1);

	return nullptr;
}
