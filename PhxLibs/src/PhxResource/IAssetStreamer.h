#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/Handle.h>
#include <PhxCore/Span.h>

namespace phx
{
	struct StreamFile;
	using StreamFileHandle = Handle<StreamFile>;
	
	struct StreamRequest
	{
		StreamFileHandle FileHandle = {};
		uint64_t Offset = 0;
		uint64_t Size = 0;
		void* Destination = nullptr;
	};

	using StreamCallback = std::function<void()>;

	class IAssetStreamer
	{
	public:
		virtual StreamFileHandle OpenFile(std::filesystem::path const& path, uint32_t statusCount) = 0;
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