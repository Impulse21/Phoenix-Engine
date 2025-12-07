#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/IResourceFileHandler.h>
#include <PhxRenderer/MeshResource.h>

namespace phx::renderer
{
	class MeshResourceHandler final : public phx::ResourceFileHandler
	{
	public:
		StringHash GetResourceTypeHash() const override { return renderer::MeshResource::StaticTypeId(); };
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		RefCountPtr<Resource> CreatePlaceholder() const override { return RefCountPtr<Resource>::Create(new MeshResource()); }
		void LoadAsync(IIoQueue* io_queue, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const override;

	private:
	};
}

PHX_DEFINE_RES_FILE_EXT(renderer::MeshResourceHandler, .phxmsh)