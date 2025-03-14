#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/Handle.h>
#include <PhxCore/Span.h>


#include <PhxResource/FileFormatUtils.h>
#include <PhxRhi/RHICore.h>

namespace phx
{
	struct StreamFile;
	using StreamFileHandle = Handle<StreamFile>;
	
	enum class DestinationType : uint64_t
	{
		Memory = 0,
		RHI_GpuBuffer,
		RHI_Texture,
		RHI_Multi_Subresource,
		RHI_TiledTexture,
	};

	struct StreamRequest
	{
		const char* DebugName = "";
		StreamFileHandle FileHandle = {};

		FileFormat::CompressionType CompressionType = FileFormat::CompressionType::None;
		uint64_t SrcSize = 0;
		uint64_t DestSize = 0;

		uint64_t Offset = 0;

		struct Destination
		{
			DestinationType Type = DestinationType::Memory;
			union
			{
				void* Memory = nullptr;
				rhi::GpuBufferHandle Buffer;
				rhi::TextureHandle Texture;
			};
		} Destination;
	};

	using StreamCallback = std::function<void()>;

	class IAssetStreamer
	{
	public:
		virtual StreamFileHandle OpenFile(std::filesystem::path const& path) = 0;
		virtual void CloseFile(StreamFileHandle handle) = 0;

		virtual bool GetStatus(StreamFileHandle handle, uint32_t status) const = 0;
		virtual uint64_t GetFileSize(StreamFileHandle handle) const = 0;

		virtual void SubmitBatch(Span<StreamRequest> requests, StreamCallback callback) = 0;
		virtual void Submit(StreamRequest request, StreamCallback callback)
		{
			SubmitBatch({ request }, callback);
		}
	};
}