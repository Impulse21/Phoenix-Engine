#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResourceHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceFile.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

#include <PhxRhi/PhxRhi.h>

void phx::renderer::TextureResourceHandler::LoadAsync(
	IIoQueue* io_queue,
	RefCountPtr<Resource> resource,
	AsyncResourceDescriptor const& resource_descriptor) const
{
	RefCountPtr<TextureResource> texture_resource = resource.As<TextureResource>();
	texture_resource->state = Resource::State::Loading;
	ResourceFile::Load(
		io_queue,
		resource_descriptor,
		[texture_resource, resource_descriptor](std::shared_ptr<ResourceFile> resource_file)
		{
			ResourceFileFormat::MetadataHeader* metadata_header = resource_file->metadata_header.Get();
			TextureMetadata* metadata_view = reinterpret_cast<TextureMetadata*>(metadata_header->MetadataChunk.Get());

			texture_resource->texture_handle = rhi::CreateTexture({
					.DebugName = resource_descriptor.virtual_path.c_str(),
					.Format = metadata_view->format,
					.Width = metadata_view->width,
					.Height = metadata_view->height,
					.ArraySize = static_cast<uint16_t>(metadata_view->array_layers),
					.MipLevels = static_cast<uint16_t>(metadata_view->mip_levels),
				});


			const ResourceFileFormat::Chunk& gpu_chunk_header = metadata_header->Chunks[0];

			StreamingOperation gpu_operation =
			{
				.source = {
					.data = resource_descriptor,
					.offset = gpu_chunk_header.Offset.Offset,
					.size = gpu_chunk_header.CompressedSize,
				},
				.destination = {
					.target = GpuResourceDestinationInfo
						{
							.handle = texture_resource->texture_handle,
					},
					.size = gpu_chunk_header.UncompressedSize,
				}
			};

			StreamingRequest request = {
			   .operations = { gpu_operation }
			};

			request.on_complete = [texture_resource](StreamingResult const& result) mutable {
				if (result.error_code != ErrorCode::Success)
				{
					texture_resource->state = Resource::State::Error;
					return;
				}

				texture_resource->state = Resource::State::On_Gpu;
			};

			IIoQueue::Ptr->Submit(std::move(request));
		},
		[texture_resource] {
			PHX_CORE_ERROR("Failed to load mesh resource.");
			texture_resource->state = Resource::State::Error;
		});

}
