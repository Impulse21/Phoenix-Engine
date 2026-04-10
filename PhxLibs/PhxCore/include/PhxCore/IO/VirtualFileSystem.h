#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/IO/IFileSystems.h>

#include <string>
#include <memory>

namespace phx
{
	namespace Vfs
	{
		void Initialize(std::unique_ptr<IRootFileSystem> root_file_system);
		void Finalize();

        FilePtr Open(const std::string& virtual_path, FileMode file_mode);
        phx::Result<PlatformFileHandle> OpenRaw(const std::string& virtual_path, FileMode file_mode);

		bool Exists(std::string const& virtual_path);
		phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& virtual_path) const;

        // -- Metadata retrival ---
		Result<std::string> ResolveVirtualToPhysicalPath(std::string const& virtual_path);
		Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path);
		Result<std::vector<std::string>> GetResourceDependencies(std::string const& virtual_path);

		Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path);
		Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path);

		bool Mount(std::string const& virtual_path, std::string const& physical_path);
		bool Unmount(std::string const& virtual_path);

		IRootFileSystem* GetFileSystem();
	}
}
