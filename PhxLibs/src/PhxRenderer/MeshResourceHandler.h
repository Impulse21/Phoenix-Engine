#pragma once

#include <PhxResource/IResource.h>
#include <PhxResource/IResourceHandler.h>

namespace phx::renderer
{
	struct MeshMetadata;
	struct MeshResource;
	class MeshResourceHandler final : public phx::ResourceHandler
	{
	public:
		RefCountPtr<Resource> LoadFromPak(
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle filehandle,
			PakFileFormat::AssetEntry const& assetEntry) const override;

		RefCountPtr<Resource> LoadLoose(
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle filehandle) const override;

	private:
		static void RequestMeshData(
			RefCountPtr<MeshResource> meshResource,
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle fileHandle,
			const MeshMetadata* meshMetadata,
			const ResourceFileFormat::Chunk* chunks);
	};
}

namespace phx
{
	template<>
	struct ResourceExtension<renderer::MeshResourceHandler>
	{
		static constexpr const char* value = ".phxmsh";
	};

	template<>
	struct ResourceHandlerId<renderer::MeshResourceHandler>
	{
		static constexpr phx::StringHash value = "phxmsh"_hash;
	};
}

