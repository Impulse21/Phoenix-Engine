#include "PhxResource/PhxResource_pch.h"
#include "ResourceFileView.h"

using namespace phx;

StreamingRequest phx::resource_utils::PrepareHeaderLoadRequest(
	ResourceFileView* resource_file_view,
	AsyncResourceDescriptor const& async_descriptor)
{
	return {
			   .operations = {
				   {
					   .source = {
						   .data = async_descriptor,
						   .size = sizeof(ResourceFileFormat::Header),
					   },
					   .destination = {
						   .target = CpuDestination{.address = &resource_file_view->header },
						   .size = sizeof(ResourceFileFormat::Header),
					   }
				   }
			   }
	};
}

StreamingRequest phx::resource_utils::PrepareMetadataLoadRequest(ResourceFileView* resource_file_view, AsyncResourceDescriptor const& async_descriptor, void* dest)
{
	return {
			.debug_name = "Resource Metadata Load Request",
			.operations = {
				{
					.source = {
						.data = async_descriptor,
						.offset = sizeof(ResourceFileFormat::Header),
						.size = resource_file_view->header.MetadataHeapSize,
					},
					.destination = {
						.target = CpuDestination{.address = dest},
						.size = resource_file_view->header.MetadataHeapSize,
					}
				}
			}
	};
}

bool phx::resource_utils::IsHeaderValid(ResourceFileView* resource_file_view)
{
	return resource_file_view->header.Magic != ResourceFileFormat::MagicNumber ||
		resource_file_view->header.Version != ResourceFileFormat::Version;
}
