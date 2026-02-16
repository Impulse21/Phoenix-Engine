#include "PhxRenderer_pch.h"

#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxRenderer/MeshResource.h>

#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceFileView.h>
#include <PhxResource/ResourceManager.h>

#include <PhxEngine/IO/IIoQueue.h>
#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
	enum InternalState
	{
		State_Init						= ResourceState::Loading,
		State_Parse_Header				= ResourceState::Loading + 1,
		State_Wait_Header				= ResourceState::Loading + 2,
		State_Parse_Metadata			= ResourceState::Loading + 3,
		State_Wait_Metadata				= ResourceState::Loading + 4,
		State_Load_Core_Data			= ResourceState::Loading + 5,
		State_Wait_Core_Data			= ResourceState::Loading + 6
	};
}

LoaderStepResult phx::renderer::MeshResourceHandler::Step(LoadContext& ctx) const
{
	RefCountPtr<MeshResource> mesh_handle = ctx.handle.As<MeshResource>();
	auto state = ctx.GetInternalState<InternalState>();

	switch (state)
	{
	case State_Init:
	{
		auto resource_file_view = ctx.GetScratch<ResourceFileView>();
		std::memset(resource_file_view, 0, sizeof(ResourceFileView));
		StreamingRequest request = resource_utils::PrepareHeaderLoadRequest(resource_file_view, ctx.resource_descriptor);

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(request));
		ctx.state_index = State_Wait_Header;

		return LoaderStepResult::Continue;
	}
	case State_Wait_Header:
	{
		auto io_queue = IIoQueue::Ptr;
		if (!io_queue->IsComplete(ctx.io_ticket))
		{
			return LoaderStepResult::Yield;
		}
		auto result = io_queue->GetResult(ctx.io_ticket);
		if (result.error_code != ErrorCode::Success)
		{
			PHX_CORE_ERROR("Failed to load mesh resourceheader.");
			return LoaderStepResult::Error;
		}

		if (!resource_utils::IsHeaderValid(ctx.GetScratch<ResourceFileView>()))
		{
			PHX_CORE_ERROR("Mesh resourcefile is invalid or corrupted.");
			return LoaderStepResult::Error;
		}

		ctx.state_index = State_Parse_Metadata;
		return LoaderStepResult::Continue;
	}
	case State_Parse_Metadata:
	{
		auto resource_file_view = ctx.GetScratch<ResourceFileView>();
		ctx.file_buffer = MemoryBuffer(resource_file_view->header.MetadataHeapSize);

		resource_file_view->metadata_header = ctx.file_buffer.GetView<ResourceFileFormat::MetadataHeader>();

		StreamingRequest metadata_request = resource_utils::PrepareMetadataLoadRequest(
			resource_file_view,
			ctx.resource_descriptor,
			ctx.file_buffer.Data());

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(metadata_request));
		ctx.state_index = State_Wait_Metadata;

		return LoaderStepResult::Continue;
	}
	case State_Wait_Metadata:
	{
		auto io_queue = IIoQueue::Ptr;
		if (!io_queue->IsComplete(ctx.io_ticket))
		{
			return LoaderStepResult::Yield;
		}

		auto result = io_queue->GetResult(ctx.io_ticket);
		if (result.error_code != ErrorCode::Success)
		{
			PHX_CORE_ERROR("Failed to load mesh resourcemetadata.");
			return LoaderStepResult::Error;
		}

		ctx.state_index = State_Load_Core_Data;
		return LoaderStepResult::Continue;
	}
	case State_Load_Core_Data:
	{
		ResourceFileFormat::MetadataHeader* metadata_header = ctx.GetScratch<ResourceFileView>()->metadata_header.Get();

		const size_t packed_mesh_buffer_size = metadata_header->Chunks[1].UncompressedSize;
		PHX_CORE_INFO("Loading mesh with packed mesh buffer size: {0} bytes", packed_mesh_buffer_size);

		mesh_handle->packed_mesh_buffer = rhi::CreateBuffer({
				.DebugName = "packed_mesh_buffer",
				.Size = static_cast<uint32_t>(packed_mesh_buffer_size),
				.BindingFlags = rhi::BindingFlags::IndexBuffer | rhi::BindingFlags::ShaderResource,
				.MiscFlags = rhi::ResourceMiscFlags::BufferRaw,
			});


		const ResourceFileFormat::Chunk& cpu_chunk_header = metadata_header->Chunks[0];
		mesh_handle->cpu_data_buffer = MemoryBuffer(cpu_chunk_header.UncompressedSize);
		mesh_handle->cpu_data = mesh_handle->cpu_data_buffer.GetView<MeshResource::CpuData>();

		StreamingOperation cpu_operation =
		{
			.source = {
				.data = ctx.resource_descriptor,
				.offset = cpu_chunk_header.Offset.Offset,
				.size = cpu_chunk_header.CompressedSize,
			},
			.destination = {
				.target = CpuDestination{.address = mesh_handle->cpu_data_buffer.Data() },
				.size = cpu_chunk_header.UncompressedSize,
			}
		};

		const ResourceFileFormat::Chunk& gpu_chunk_header = metadata_header->Chunks[1];

		StreamingOperation gpu_operation =
		{
			.source = {
				.data = ctx.resource_descriptor,
				.offset = gpu_chunk_header.Offset.Offset,
				.size = gpu_chunk_header.CompressedSize,
			},
			.destination = {
				.target = GpuBufferDestination{.handle = mesh_handle->packed_mesh_buffer, },
				.size = gpu_chunk_header.UncompressedSize,
			}
		};

		StreamingRequest request = {
			.debug_name = "Mesh CPU and GPU data request",
		   .operations = { cpu_operation, gpu_operation }
		};

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(request));
		ctx.state_index = State_Wait_Core_Data;

		return LoaderStepResult::Continue;
	}
	case State_Wait_Core_Data:
	{
		auto io_queue = IIoQueue::Ptr;
		if (!io_queue->IsComplete(ctx.io_ticket))
		{
			return LoaderStepResult::Yield;
		}
		auto result = io_queue->GetResult(ctx.io_ticket);
		if (result.error_code != ErrorCode::Success)
		{
			PHX_CORE_ERROR("Failed to load mesh resource data onto GPU.");
			return LoaderStepResult::Error;
		}

		return LoaderStepResult::WaitOnGpuTransition;
	}
	default:
	{
		throw std::runtime_error("Invalid mesh loader state.");
	}
	}

	throw std::runtime_error("Invalid mesh loader state.");

}