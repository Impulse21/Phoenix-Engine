#include "PhxResource/PhxResource_pch.h"
#include "ResourceFile.h"

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxEngine/IO/IIoQueue.h>

using namespace phx;

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

void phx::ResourceFile::PrepareRequest(
	StreamingRequest& request,
	IIoQueue* io_queue,
	AsyncResourceDescriptor const& resource_descriptor,
	MetadataLoadCallbackFunc metadata_loaded_callback,
	FailureCallbackFunc failure_callback)
{
	std::shared_ptr<phx::ResourceFile> resource_file = std::make_shared<phx::ResourceFile>();
	resource_file->io_queue = io_queue;
	resource_file->resource_descriptor = resource_descriptor;
	resource_file->metadata_loaded_callback = std::move(metadata_loaded_callback);
	resource_file->failure_callack = failure_callback;

	request = {
		   .operations = {
			   {
				   .source = {
					   .data = resource_descriptor,
					   .size = sizeof(ResourceFileFormat::Header),
				   },
				   .destination = {
					   .target = CpuDestination{ .address= &resource_file->header },
					   .size = sizeof(ResourceFileFormat::Header),
				   }
			   }
		   }
	};


	request.on_complete = [resource_file, io_queue](StreamingResult const& result) mutable {
		if (result.error_code != ErrorCode::Success)
		{
			resource_file->failure_callack();
		}

		if (resource_file->header.Magic != ResourceFileFormat::MagicNumber ||
			resource_file->header.Version != ResourceFileFormat::Version)
		{
			return;
		}

		// TODO: Load data chunks (metadata, cpu, gpu)
		resource_file->metadata_buffer = MemoryBuffer(resource_file->header.MetadataHeapSize);
		resource_file->metadata_header = resource_file->metadata_buffer.GetView<ResourceFileFormat::MetadataHeader>();
		StreamingRequest metadata_request = {
			.debug_name = "Resource Metadata Load Request",
			.operations = {
				{
					.source = {
						.data = resource_file->resource_descriptor,
						.offset = sizeof(ResourceFileFormat::Header),
						.size = resource_file->header.MetadataHeapSize,
					},
					.destination = {
						.target = CpuDestination{ .address = resource_file->metadata_buffer.Data() },
						.size = resource_file->header.MetadataHeapSize,
					}
				}
			}
		};
		metadata_request.on_complete = [resource_file](StreamingResult const& metadata_result) mutable {
			if (metadata_result.error_code != ErrorCode::Success)
			{
				resource_file->failure_callack();
				return;
			}
			resource_file->metadata_loaded_callback(resource_file);
		};

		io_queue->Submit(std::move(metadata_request));
	};
}