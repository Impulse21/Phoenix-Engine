#include "PhxRenderer/PhxRenderer_pch.h"

#include "MeshResourceHandler.h"
#include "MeshResource.h"

#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceSystem.h>

#include <PhxEngine/IO/IIoQueue.h>
#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;

void phx::renderer::MeshResourceHandler::LoadAsync(IIoQueue* io_queue, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const
{
	// TODO: Check if cached version is loaded already. If so, load from there.
	RefCountPtr<MeshResource> mesh_resource = resource.As<MeshResource>();

	mesh_resource->state = Resource::State::Loading;
	ResourceFile::Load(
		io_queue,
		resource_descriptor,
		[mesh_resource, resource_descriptor](std::shared_ptr<ResourceFile> resource_file)
		{
			ResourceFileFormat::MetadataHeader* metadata_header = resource_file->metadata_header.Get();
			MeshMetadata* metadata_view = reinterpret_cast<MeshMetadata*>(metadata_header->MetadataChunk.Get());
			PHX_CORE_INFO("Loading mesh with packed mesh buffer size: {0} bytes", metadata_view->packed_mesh_buffer);

			mesh_resource->packed_mesh_buffer = rhi::CreateBuffer({
					.DebugName = "packed_mesh_buffer",
					.Size = metadata_view->packed_mesh_buffer,
					.BindingFlags = rhi::BindingFlags::IndexBuffer | rhi::BindingFlags::ShaderResource,
					.MiscFlags = rhi::ResourceMiscFlags::BufferRaw,
				});
			

			const ResourceFileFormat::Chunk& cpu_chunk_header = metadata_header->Chunks[0];
			mesh_resource->cpu_data_buffer = MemoryBuffer(cpu_chunk_header.UncompressedSize);
			mesh_resource->cpu_data = mesh_resource->cpu_data_buffer.GetView<MeshResource::CpuData>();

			StreamingOperation cpu_operation =
			{
				.source = {
					.data = resource_descriptor,
					.offset = cpu_chunk_header.Offset.Offset,
					.size = cpu_chunk_header.CompressedSize,
				},
				.destination = {
					.target = CpuResourceDestinationInfo{ .handle = mesh_resource->cpu_data_buffer.Data()},
					.size = cpu_chunk_header.UncompressedSize,
				}
			};

			const ResourceFileFormat::Chunk& gpu_chunk_header = metadata_header->Chunks[1];

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
							.handle = mesh_resource->packed_mesh_buffer,
					},
					.size = gpu_chunk_header.UncompressedSize,
				}
			};

			StreamingRequest request = {
			   .operations = { cpu_operation, gpu_operation }
			};

			request.on_complete = [mesh_resource](StreamingResult const& result) mutable {
				if (result.error_code != ErrorCode::Success)
				{
					mesh_resource->state = Resource::State::Error;
					return;
				}

				mesh_resource->state = Resource::State::On_Gpu;
			};

			IIoQueue::Ptr->Submit(std::move(request));
		},
		[mesh_resource] {
			PHX_CORE_ERROR("Failed to load mesh resource.");
			mesh_resource->state = Resource::State::Error;
		});

}