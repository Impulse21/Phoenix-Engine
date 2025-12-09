#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/MaterialResource.h>

namespace phx::renderer
{
	class MaterialResourceHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		void PrepareRequest(StreamingRequest& request, GenericHandle handle, phx::IIoQueue* queue, AsyncResourceDescriptor const& resource_descriptor) const override;

	private:
	};
}