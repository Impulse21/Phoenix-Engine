#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/IResourceFileHandler.h>
#include "ModelResoure.h"

namespace phx::renderer
{
	struct ModelMetadata;

	class ModelResourceHandler final : public phx::ResourceFileHandler
	{
	public:
		StringHash GetResourceTypeHash() const override { return renderer::ModelResoure::StaticTypeHash(); };
		bool IsStale(std::string const&, IVirtualFileSystem*) const override { return false; }
		RefCountPtr<Resource> CreatePlaceholder() const override { return RefCountPtr<Resource>::Create(new ModelResoure()); }
		void LoadAsync(IStreamingManager* streaming_manager, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const override;

	private:
#if false
		static void RequestMeshData(
			RefCountPtr<ModelResoure> modelResoure,
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle fileHandle,
			const ModelMetadata* metadata,
			const ResourceFileFormat::Chunk* chunks);
#endif
	};
}

PHX_DEFINE_RES_FILE_EXT(renderer::ModelResourceHandler, .phxmdl)