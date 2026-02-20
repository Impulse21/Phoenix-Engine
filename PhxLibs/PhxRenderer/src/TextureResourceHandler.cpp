#include "PhxRenderer_pch.h"
#include "TextureResourceHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceFileView.h>
#include <PhxResource/ResourceManager.h>

#include <PhxRenderer/TextureResource.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;

namespace
{
	enum InternalState
	{
		State_Init						= ResourceState::Loading,
		State_Parse_Header				= ResourceState::Loading + 1,
		State_Wait_Header				= ResourceState::Loading + 2,
		State_Parse_Metadata			= ResourceState::Loading + 3,
		State_Wait_Metadata				= ResourceState::Loading + 4,
		State_Upload_Gpu_Data			= ResourceState::Loading + 5,
		State_Wait_Gpu_Data				= ResourceState::Loading + 6
	};
}

LoaderStepResult phx::renderer::TextureResourceHandler::Step(LoadContext& ctx) const
{
	RefCountPtr<TextureResource> tex_handle = ctx.handle.As<TextureResource>();
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
			PHX_CORE_ERROR("Failed to load texture resource header.");
			return LoaderStepResult::Error;
		}

		if (!resource_utils::IsHeaderValid(ctx.GetScratch<ResourceFileView>()))
		{
			PHX_CORE_ERROR("Texture resource file is invalid or corrupted.");
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
			PHX_CORE_ERROR("Failed to load texture resource metadata.");
			return LoaderStepResult::Error;
		}

		ctx.state_index = State_Upload_Gpu_Data;

		return LoaderStepResult::Continue;
	}
	case State_Upload_Gpu_Data:
	{
		ResourceFileFormat::MetadataHeader* metadata_header = ctx.GetScratch<ResourceFileView>()->metadata_header.Get();
		TextureMetadata* metadata_view = reinterpret_cast<TextureMetadata*>(metadata_header->MetadataChunk.Get());

		tex_handle->texture_handle = rhi::CreateTexture({
				.DebugName = ctx.resource_descriptor.virtual_path.c_str(),
				.Format = metadata_view->format,
				.Width = metadata_view->width,
				.Height = metadata_view->height,
				.ArraySize = static_cast<uint16_t>(metadata_view->array_layers),
				.MipLevels = static_cast<uint16_t>(metadata_view->mip_levels),
			});

		const ResourceFileFormat::Chunk& gpu_chunk_header = metadata_header->Chunks[0];

		StreamingRequest request = {
			.debug_name = "Texture GPU Load",
			.operations = {
			   {
				   .source = {
						.data = ctx.resource_descriptor,
						.offset = gpu_chunk_header.Offset.Offset,
						.size = gpu_chunk_header.CompressedSize,
					},
					.destination = {
						.target = GpuTextureDestination{
							.handle = tex_handle->texture_handle,
						},
					.size = gpu_chunk_header.UncompressedSize,
					}
			   }
			}
		};

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(request));
		ctx.state_index = State_Wait_Gpu_Data;

		return LoaderStepResult::Continue;
	}
	case State_Wait_Gpu_Data:
	{
		auto io_queue = IIoQueue::Ptr;
		if (!io_queue->IsComplete(ctx.io_ticket))
		{
			return LoaderStepResult::Yield;
		}

		auto result = io_queue->GetResult(ctx.io_ticket);
		if (result.error_code != ErrorCode::Success)
		{
			PHX_CORE_ERROR("Failed to load texture resource data onto GPU.");
			return LoaderStepResult::Error;
		}

		return LoaderStepResult::WaitOnGpuTransition;
	}
	default:
	{
		throw std::runtime_error("Invalid texture loader state.");
	}
	}

	throw std::runtime_error("Invalid texture loader state.");
}
