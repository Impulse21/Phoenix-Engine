#include "PhxRenderer_pch.h"

#include "MeshResourceHandler.h"
#include "MeshResource.h"

#include <PhxResource/ResourceFileFormat.h>

using namespace phx;
using namespace phx::renderer;

RefCountPtr<IResource> phx::renderer::MeshResourceHandler::Load(std::shared_ptr<IAssetStreamer> const& /*assetStreamer*/, StreamFileHandle /*filehandle*/, PakFileFormat::AssetEntry const& assetEntry) const
{
	auto meshCpuData = reinterpret_cast<const MeshMetadata*>(assetEntry.MetadataChunk.Get());

	assert(meshCpuData->GeometryBufferSize > 0);

	return nullptr;
}
