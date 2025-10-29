#pragma once

#include <unordered_map>

#include <PhxCore/Span.h>

#include <PhxResource/Resource.h>
#include <PhxResource/IResourceFileHandler.h>

#include <PhxWorld/PrefabResource.h>

struct cgltf_mesh;

namespace phx
{
	struct AsyncResourceDescriptor;

	class GltfPrefabHandler final : public phx::ResourceFileHandler
	{
	public:
		StringHash GetResourceTypeHash() const override { return PrefabResource::StaticTypeHash(); };
		bool IsStale(std::string const& virtual_file_path, IVirtualFileSystem* vfs) const override;
		RefCountPtr<Resource> CreatePlaceholder() const override { return RefCountPtr<Resource>::Create(new PrefabHandleResource()); }
		void LoadAsync(IStreamingManager* streaming_manager, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const override;

	private:
		static void CookPrefab(RefCountPtr<PrefabHandleResource> prefab_handle_resource, AsyncResourceDescriptor const& resource_descriptor, void* file_data);
	};
}


PHX_DEFINE_RES_FILE_EXT(GltfPrefabHandler, .gltf)