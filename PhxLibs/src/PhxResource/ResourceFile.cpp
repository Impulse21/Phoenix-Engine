#include "PhxResource/PhxResource_pch.h"
#include "ResourceFile.h"

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>

using namespace phx;
using namespace phx::data;

namespace
{
	namespace StreamingStatus
	{
		enum : uint32_t
		{
			Metadata = 0,
			CpuData,
			GpuData,
			NumEntries
		};
	}
}

void phx::ResourceFile::Load(data::IStreamingManager* streaming_manager, data::AsyncResourceDescriptor const& resource_descriptor, MetadataLoadCallbackFunc metadata_loaded_callback)
{
	std::shared_ptr<phx::ResourceFile> resource_file = std::make_shared<phx::ResourceFile>();
	resource_file->streaming_manager = streaming_manager;
	resource_file->resource_descriptor = resource_descriptor;
	resource_file->metadata_loaded_callback = std::move(metadata_loaded_callback);

	data::StreamingRequest request = {
		   .operations = {
			   {
				   .source = {
					   .data = resource_descriptor,
					   .size = sizeof(ResourceFileFormat::Header),
				   },
				   .destination = {
					   .target = &resource_file->header,
					   .size = sizeof(ResourceFileFormat::Header),
				   }
			   }
		   }
	};

	request.on_complete = [resource_file](data::StreamingResult const& result) mutable {
		if (result.error_code != ErrorCode::Success)
		{
			resource_file->failure_callack();
		}

		if (resource_file->header.Magic != ResourceFileFormat::MagicNumber ||
			resource_file->header.Version != ResourceFileFormat::Version)
		{
			return;
		}
#if false
		data::StreamingRequest request = {
			   .operations = {
				   {
					   .source = {
						   .data = resource_file->resource_descriptor,
						   .offset = sizeof(ResourceFileFormat::Header),
						   .size = resource_file->header.MetadataHeapSize,
					   },
					   .destination = {
							.target = resource_file->metadata,
							.size = resource_file->header.MetadataHeapSize,
					   }
				   }
			   }
		};

		request.on_complete = [resource_file](data::StreamingResult const& result) mutable {
			if (result.error_code != ErrorCode::Success)
			{
				resource_file->failure_callack();
			}

			resource_file->metadata_loaded_callback(resource_file);
			};
		};
#endif
		};
}