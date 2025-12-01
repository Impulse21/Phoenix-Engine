#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/IResourceFileHandler.h>
#include <PhxRenderer/MaterialResource.h>

namespace phx::renderer
{
	class MaterialResourceHandler final : public phx::ResourceFileHandler
	{
	public:
		StringHash GetResourceTypeHash() const override { return renderer::TextureResource::StaticTypeId(); };
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		RefCountPtr<Resource> CreatePlaceholder() const override { return RefCountPtr<Resource>::Create(new TextureResource()); }
		void LoadAsync(IIoQueue* io_queue, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const override;

	private:
	};
}

PHX_DEFINE_RES_FILE_EXT(renderer::MaterialResourceHandler, .phxmat)