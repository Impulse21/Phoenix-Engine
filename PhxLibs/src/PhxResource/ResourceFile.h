#pragma once

#include "Resource.h"
#include "ResourceFileFormat.h"

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxData/StreamingDefintions.h>

#include <functional>

namespace phx
{
	struct ResourceFile;
	using MetadataLoadCallbackFunc = std::function<void(std::shared_ptr<ResourceFile>)>;
	using FailureCallbackFunc = std::function<void()>;

	namespace data
	{
		class IStreamingManager;
	}

	struct ResourceFile
	{
		data::IStreamingManager* streaming_manager;
		data::AsyncResourceDescriptor resource_descriptor;
		ResourceFileFormat::Header header = {};
		MemoryBuffer metadata;

		MetadataLoadCallbackFunc metadata_loaded_callback;
		FailureCallbackFunc failure_callack;

		static void Load(
			data::IStreamingManager* streaming_manager,
			data::AsyncResourceDescriptor const& resource_descriptor,
			MetadataLoadCallbackFunc metadata_loaded_callback);
	};
}

