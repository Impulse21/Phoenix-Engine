#pragma once

#include <PhxCore/VFS.h>
#include <PhxCore/Handle.h>

#include <functional>

namespace phx
{
	struct File;
	using FileHandle = Handle<File>;

	class IResourceFileSystem : public IRootFileSystem
	{
	public:
		using RequestCallbackFunc = std::function<void()>;
	public:
		virtual ~IResourceFileSystem() = default;

		virtual FileHandle Open(std::filesystem::path const& path) = 0;
		virtual void Close(FileHandle handle) = 0;

		virtual void EnqueueRead(FileHandle handle, uint64_t offset, uint64_t size, void* dest, RequestCallbackFunc&& callback) = 0;

		template<typename T>
		void EnqueueRead(FileHandle handle, uint64_t offset, T* dest, RequestCallbackFunc&& func)
		{
			EnqueueRead(handle, offset, sizeof(T), dest, func);
		}
		template<typename T>
		void EnqueueReadArray(FileHandle handle, uint64_t offset, T* dest, size_t numEntries, RequestCallbackFunc&& callback)
		{
			EnqueueRead(handle, offset, sizeof(T) * numEntries, dest, callback);
		}

		virtual std::unique_ptr<IBlob> EnqueueReadBlob(FileHandle handle, uint64_t offset, uint64_t size, RequestCallbackFunc&& callback) = 0;

		virtual void SubmitRequests() = 0;
	};


	namespace FileSystemFactory
	{
		std::unique_ptr<IResourceFileSystem> CreateResourceFileSystem();
	}
}