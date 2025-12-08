#pragma once

#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

namespace phx
{
	class IVirtualFileSystem;
	struct AsyncResourceDescriptor;
	struct StreamingRequest;
	class IOQueue;

	class IResourceLoader
	{
	public:
		virtual ~IResourceLoader() = default;
		virtual bool IsStale(AsyncResourceDescriptor const& resource_descriptor, IVirtualFileSystem* vfs) const = 0;
		virtual void PrepareRequest(StreamingRequest& request, GenericHandle handle, IOQueue* queue, AsyncResourceDescriptor const& resource_descriptor) const = 0;

	};
}