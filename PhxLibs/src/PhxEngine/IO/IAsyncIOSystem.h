#pragma once

#include <PhxCore/VFS.h>
#include <PhxCore/IO/MemoryRegion.h>
namespace phx
{
	struct AsyncReadResult;
	struct AsyncReadRequest
	{
		FileHandle Handle;
		uint64_t Offset = 0;
		uint64_t BytesToRead = 0;
		std::function<void(AsyncReadResult const&)> Callback;
		void* UserContext;
		// TODO GPU Stuff
	};

	struct AsyncReadResults
	{
		MemoryRegion<uint8_t> Data;
		uint64_t BytesRead;
		void* UserContext;
	};

	class IAsyncIOSystem
	{
	public:
		inline static IAsyncIOSystem* Ptr = nullptr;
	public:
		~IAsyncIOSystem() = default;

		virtual bool Initialize() = 0;

		virtual void Shutdown() = 0;
		virtual void QueueRead(AsyncReadRequest&& request) = 0;
	};


}