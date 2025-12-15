#pragma once

#include "ResourceFileFormat.h"

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxEngine/StreamingDefintions.h>

#include <functional>

namespace phx
{
	struct ResourceFile;
	using MetadataLoadCallbackFunc = std::function<void(std::shared_ptr<ResourceFile>)>;
	using FailureCallbackFunc = std::function<void()>;

	class IIoQueue;

	struct ResourceFileView
	{
		ResourceFileFormat::Header header = {};
		TypedView<ResourceFileFormat::MetadataHeader> metadata_header;
	};

	namespace resource_utils
	{
		StreamingRequest PrepareHeaderLoadRequest(ResourceFileView* resource_file_view, AsyncResourceDescriptor const& async_descriptor);
		StreamingRequest PrepareMetadataLoadRequest(ResourceFileView* resource_file_view, AsyncResourceDescriptor const& async_descriptor, void* dest);
		bool IsHeaderValid(ResourceFileView* resource_file_view);
	}
}

