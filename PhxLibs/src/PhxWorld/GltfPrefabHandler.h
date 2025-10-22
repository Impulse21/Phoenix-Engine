#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/IResourceFileHandler.h>

#include <PhxWorld/PrefabResource.h>

namespace phx
{
	class GltfPrefabHandler final : public phx::ResourceFileHandler
	{
	public:
		StringHash GetResourceTypeHash() const override { return PrefabResource::StaticTypeHash(); };
		RefCountPtr<Resource> CreatePlaceholder() const override { return RefCountPtr<Resource>::Create(new PrefabResource()); }
		void LoadAsync(IStreamingManager* streaming_manager, IVirtualFileSystem* vfs, RefCountPtr<Resource> resource, std::string const& virtual_file_path) const override;

	};
}

