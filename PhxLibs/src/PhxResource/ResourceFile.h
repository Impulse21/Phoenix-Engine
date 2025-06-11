#pragma once

#include "Resource.h"
#include "ResourceFileFormat.h"
#include "IAssetStreamer.h"

#include <PhxCore/IO/MemoryRegion.h>
#include <functional>

namespace phx
{
	struct ResourceFile;
	using MetadataLoadCallbackFunc = std::function<void(std::shared_ptr<ResourceFile>)>;
	using FailureCallbackFunc = std::function<void()>;

	struct ResourceFile
	{
		std::shared_ptr<IAssetStreamer> AssetStreamer;
		StreamFileHandle FileHandle;
		ResourceFileFormat::Header Header = {};
		MemoryRegion<ResourceFileFormat::MetadataHeader> Metadata;

		MetadataLoadCallbackFunc MetadataLoadedCallback;
		FailureCallbackFunc FailureCallback;

		static void Load(
			std::shared_ptr<IAssetStreamer> assetStreamer,
			StreamFileHandle fileHandle,
			MetadataLoadCallbackFunc metadataLoadedCallback);
	};
}

