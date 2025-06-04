#pragma once

#include <string>
#include <memory>
#include <PhxData/AsyncIODefinitions.h>
#include <PhxCore/Platform/PlatformWrapper.h>


namespace phx::data
{
	struct Blob
	{
		std::unique_ptr<void> Data;
		size_t Size;
	};

	class IVirtualFileSystem
	{
	public:
		inline static IVirtualFileSystem* Ptr = nullptr;
	public:
		virtual ~IVirtualFileSystem() = default;

		virtual bool Mount(std::string const& virtual_path, std::string const& physical_path) = 0;
		virtual bool Unmount(std::string const& virtual_path) = 0;

		virtual Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path) const = 0;

		virtual Result<platform::PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path) const = 0;

		virtual bool Exists(std::string const& virtual_path) = 0;
		virtual Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path) const = 0;
		virtual Result<Blob> ReadFileSynchronous(const std::string& virtual_path) const = 0;
	};
}