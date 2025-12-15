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
}

