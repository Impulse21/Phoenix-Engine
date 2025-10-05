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
		RefCountPtr<Resource> CreatePlaceholder() const override { return RefCountPtr<Resource>::Create(new ModelResoure()); }
		void LoadAsync(ResourceSystem* resource_system, RefCountPtr<Resource> asset, std::string const& virtual_file_path) const override;

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