#include "PhxResource/PhxResource_pch.h"
#include "ResourceFile.h"
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
void phx::ResourceFile::Load(
	std::shared_ptr<IAssetStreamer> assetStreamer,
	StreamFileHandle fileHandle,
	MetadataLoadCallbackFunc metadataLoadedCallback)
{
	std::shared_ptr<phx::ResourceFile> resourceFile = std::make_shared<phx::ResourceFile>();
	resourceFile->AssetStreamer = std::move(assetStreamer);
	resourceFile->FileHandle = fileHandle;
	resourceFile->MetadataLoadedCallback = std::move(metadataLoadedCallback);

	assetStreamer->Submit({
			.DebugName = "Resource Header Load",
			.FileHandle = fileHandle,
			.SrcSize = sizeof(ResourceFileFormat::Header),
			.DestSize = sizeof(ResourceFileFormat::Header),
			.Destination = {.Memory = &resourceFile->Header }
		},
		[resourceFile]
		{
			bool status =
				resourceFile->AssetStreamer->GetStatus(resourceFile->FileHandle, StreamingStatus::Metadata);

			if (!status ||
				resourceFile->Header.Magic != ResourceFileFormat::MagicNumber ||
				resourceFile->Header.Version != ResourceFileFormat::Version)
			{
				return;
			}

			StreamRequest request = StreamRequest::Create<ResourceFileFormat::MetadataHeader>(
				resourceFile->FileHandle,
				sizeof(ResourceFileFormat::Header),
				resourceFile->Header.MetadataHeapSize,
				resourceFile->Metadata);

			resourceFile->AssetStreamer->Submit(
				request,
				[resourceFile] 
				{ 
					resourceFile->MetadataLoadedCallback(resourceFile);
				});
		});
}
