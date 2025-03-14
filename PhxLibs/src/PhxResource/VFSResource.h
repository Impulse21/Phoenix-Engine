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
		FileHandle Handle = {};
		uint64_t Offset = 0;
		uint64_t Size = 0;
		void* Dest = nullptr;
	};

	class IResourceFileSystem : public IRootFileSystem
	{
	public:
		using RequestCallbackFunc = std::function<void()>;
	public:
		virtual ~IResourceFileSystem() = default;

		virtual FileHandle Open(std::filesystem::path const& path) = 0;
		virtual void Close(FileHandle handle) = 0;

		virtual void EnqueueRead(ReadRequest const& request) = 0;

		template<typename T>
		MemoryRegion<T> EnqueueReadRegion(FileHandle handle, uint64_t offset, uint32_t size)
		{
			MemoryRegion<T> dest(std::make_unique<char[]>(size));
			EnqueueRead({
					.Handle = handle,
					.Offset = offset,
					.Size = size,
					.Dest = dest.Data(),
				});

			return dest;
		};

		virtual size_t GetFileSize(FileHandle handle) const = 0;
		virtual void SubmitRequests(RequestCallbackFunc&& callback) = 0;
	};

	namespace FileSystemFactory
	{
		std::unique_ptr<IResourceFileSystem> CreateResourceFileSystem();
	}
}