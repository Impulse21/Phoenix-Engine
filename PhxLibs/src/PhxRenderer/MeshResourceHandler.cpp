#include "PhxRenderer/PhxRenderer_pch.h"

#include "MeshResourceHandler.h"
#include "MeshResource.h"

#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceManager.h>

#include <PhxEngine/IO/IIoQueue.h>
#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;

void phx::renderer::MeshResourceHandler::PrepareRequest(
	StreamingRequest& request,
	GenericHandle handle,
	phx::IIoQueue* queue,
	AsyncResourceDescriptor const& resource_descriptor) const
{
	Handle<MeshResource> mesh_handle = handle.To<MeshResource>();

	auto mesh_hot_data = ResourceStore<MeshResource>::GetHot(mesh_handle);
	mesh_hot_data->state = ResourceState::Loading;
	ResourceFile::PrepareRequest(
		request,
		queue,
		resource_descriptor,
		[mesh_handle, resource_descriptor](std::shared_ptr<ResourceFile> resource_file)
		{
			auto mesh_resource = ResourceStore<MeshResource>::GetHot(mesh_handle);

			ResourceFileFormat::MetadataHeader* metadata_header = resource_file->metadata_header.Get();
			const size_t packed_mesh_buffer_size = metadata_header->Chunks[1].UncompressedSize;
			PHX_CORE_INFO("Loading mesh with packed mesh buffer size: {0} bytes", packed_mesh_buffer_size);

			mesh_resource->packed_mesh_buffer = rhi::CreateBuffer({
					.DebugName = "packed_mesh_buffer",
					.Size = static_cast<uint32_t>(packed_mesh_buffer_size),
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

			request.on_complete = [mesh_handle](StreamingResult const& result) mutable {

				auto mesh_resource = ResourceStore<MeshResource>::GetHot(mesh_handle);
				if (result.error_code != ErrorCode::Success)
				{
					mesh_resource->state = ResourceState::Error;
					return;
				}

				mesh_resource->state = ResourceState::On_Gpu;
			};

			IIoQueue::Ptr->Submit(std::move(request));
		},
		[mesh_handle] {

			PHX_CORE_ERROR("Failed to load mesh resource.");

			auto mesh_hot_data = ResourceStore<MeshResource>::GetHot(mesh_handle);
			mesh_hot_data->state = ResourceState::Error;
		});

}