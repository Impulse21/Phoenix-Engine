#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResourceHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceManager.h>

#include <PhxRenderer/TextureResource.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;

namespace
{
	enum TextureInternalState
	{
		State_Init						= ResourceState::Loading,
		State_Parse_Header				= ResourceState::Loading + 1,
		State_Wait_Header				= ResourceState::Loading + 2,
		State_Parse_Metadata			= ResourceState::Loading + 3,
		State_Wait_Metadata				= ResourceState::Loading + 4,
		State_Upload_Texture_Data		= ResourceState::Loading + 5,
		State_Wait_Texture_Data_On_GPU	= ResourceState::Loading + 6
	};
}

LoaderStepResult phx::renderer::TextureResourceHandler::Step(LoadContext& ctx) const
{
	Handle tex_handle = ctx.handle.To<TextureResource>();
	{
		auto texture_resource = ResourceStore<TextureResource>::GetHot(tex_handle);
		texture_resource->state = ResourceState::Loading;
	}

	auto state = ctx.GetInternalState<TextureInternalState>();
	switch (state)
	{
	case State_Init:
	{
		auto resource_file_view = ctx.GetScratch<ResourceFileView>();
		std::memset(resource_file_view, 0, sizeof(ResourceFileView));
		StreamingRequest request = {
			   .operations = {
				   {
					   .source = {
						   .data = ctx.resource_descriptor,
						   .size = sizeof(ResourceFileFormat::Header),
					   },
					   .destination = {
						   .target = CpuDestination{.address = &resource_file_view->header },
						   .size = sizeof(ResourceFileFormat::Header),
					   }
				   }
			   }
		};

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
			auto texture_resource = ResourceStore<TextureResource>::GetHot(tex_handle);
			texture_resource->state = ResourceState::Error;

			return LoaderStepResult::Error;
		}

		auto resource_file_view = ctx.GetScratch<ResourceFileView>();
		if (resource_file_view->header.Magic != ResourceFileFormat::MagicNumber ||
			resource_file_view->header.Version != ResourceFileFormat::Version)
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
		const size_t metadata_size = resource_file_view->header.MetadataHeapSize;

		ctx.file_buffer = MemoryBuffer(metadata_size);
		resource_file_view->metadata_header = ctx.file_buffer.GetView<ResourceFileFormat::MetadataHeader>();

		StreamingRequest metadata_request = {
			.debug_name = "Resource Metadata Load Request",
			.operations = {
				{
					.source = {
						.data = ctx.resource_descriptor,
						.offset = sizeof(ResourceFileFormat::Header),
						.size = metadata_size,
					},
					.destination = {
						.target = CpuDestination{.address = ctx.file_buffer.Data()},
						.size = metadata_size,
					}
				}
			}
		};

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(metadata_request));

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

		return LoaderStepResult::Continue;
	}
	case State_Upload_Texture_Data:
	{
		auto texture_resource = ResourceStore<TextureResource>::GetHot(tex_handle);

		ResourceFileFormat::MetadataHeader* metadata_header = ctx.GetScratch<ResourceFileView>()->metadata_header.Get();
		TextureMetadata* metadata_view = reinterpret_cast<TextureMetadata*>(metadata_header->MetadataChunk.Get());

		texture_resource->texture_handle = rhi::CreateTexture({
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
							.handle = texture_resource->texture_handle,
						},
					.size = gpu_chunk_header.UncompressedSize,
					}
			   }
			}
		};

		ctx.io_ticket = IIoQueue::Ptr->Submit(std::move(request));
		return LoaderStepResult::Continue;
	}
	case State_Wait_Texture_Data_On_GPU:
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

		auto texture_resource = ResourceStore<TextureResource>::GetHot(tex_handle);
		texture_resource->state = ResourceState::Copied_to_gpu;
		ResourceManager::PushToGpuTransitionQueue(ctx.handle);

		return LoaderStepResult::Done;
	}
	default:
	{
		throw std::move(std::runtime_error("Invalid texture loader state."));
	}
	}

	throw std::move(std::runtime_error("Invalid texture loader state."));
}
