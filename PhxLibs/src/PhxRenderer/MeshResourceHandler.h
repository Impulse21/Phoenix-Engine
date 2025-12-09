#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/MeshResource.h>

namespace phx::renderer
{
	class MeshResourceHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		virtual void PrepareRequest(StreamingRequest& request, GenericHandle handle, phx::IIoQueue* queue, AsyncResourceDescriptor const& resource_descriptor) const override;

	private:
	};
}