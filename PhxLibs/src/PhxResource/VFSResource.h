#pragma once

#include <PhxCore/VFS.h>
#include <PhxCore/Handle.h>
#include <PhxCore/IO/MemoryRegion.h>

#include <functional>

namespace phx
{
	struct File;
	using FileHandle = Handle<File>;

	struct ReadRequest
	{
		FileHandle Handle;
		uint64_t Offset;
		uint64_t Size;
		uint8_t Status;
		void* Dest;
	};

	class IResourceFileSystem : public IRootFileSystem
	{
	public:
		using RequestCallbackFunc = std::function<void()>;
	public:
		virtual ~IResourceFileSystem() = default;

		virtual FileHandle Open(std::filesystem::path const& path) = 0;
		virtual void Close(FileHandle handle) = 0;

		virtual void EnqueueRead(ReadRequest const& request, RequestCallbackFunc&& callback) = 0;

		virtual MemoryRegion<uint8_t> EnqueueReadBlob(ReadRequest const& request, RequestCallbackFunc&& callback) = 0;

		virtual void SubmitRequests() = 0;
	};


	namespace FileSystemFactory
	{
		std::unique_ptr<IResourceFileSystem> CreateResourceFileSystem();
	}
}