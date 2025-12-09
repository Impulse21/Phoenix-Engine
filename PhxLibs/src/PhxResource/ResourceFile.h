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

	struct ResourceFile
	{
		IIoQueue* io_queue;
		AsyncResourceDescriptor resource_descriptor;
		ResourceFileFormat::Header header = {};
		MemoryBuffer metadata_buffer;
		TypedView<ResourceFileFormat::MetadataHeader> metadata_header;

		MetadataLoadCallbackFunc metadata_loaded_callback;
		FailureCallbackFunc failure_callack;

		static void PrepareRequest(
			StreamingRequest& request,
			IIoQueue* io_queue,
			AsyncResourceDescriptor const& resource_descriptor,
			MetadataLoadCallbackFunc metadata_loaded_callback,
			FailureCallbackFunc failure_callback);
	};
}

