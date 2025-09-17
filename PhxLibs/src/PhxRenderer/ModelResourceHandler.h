#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/IResourceHandler.h>

namespace phx::renderer
{
	struct ModelMetadata;
	struct ModelResoure;

	class ModelResourceHandler final : public phx::ResourceHandler
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
			RefCountPtr<ModelResoure> modelResoure,
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle fileHandle,
			const ModelMetadata* metadata,
			const ResourceFileFormat::Chunk* chunks);
	};
}

namespace phx
{
	template<>
	struct ResourceExtension<renderer::ModelResourceHandler>
	{
		static constexpr const char* value = ".phxmdl";
	};

	template<>
	struct ResourceHandlerId<renderer::ModelResourceHandler>
	{
		static constexpr phx::StringHash value = "phxmdl"_hash;
	};
}