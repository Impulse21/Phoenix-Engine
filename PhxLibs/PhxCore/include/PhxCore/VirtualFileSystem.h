#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/IO/IFileSystems.h>

#include <string>
#include <memory>

namespace phx
{
	class Vfs
	{
	public:
		static void Initialize(std::shared_ptr<IRootFileSystem> root_file_system)
		{
			ms_root_fs = std::move(root_file_system);
		}

		static void Finalize()
		{
			ms_root_fs.reset();
		}

        static FilePtr Open(const std::string& virtual_path, FileMode file_mode)
		{
			return ms_root_fs->Open(virtual_path, file_mode);
		}

        static phx::Result<PlatformFileHandle> OpenRaw(const std::string& virtual_path, FileMode file_mode)
		{
			ms_root_fs->OpenRaw(virtual_path, file_mode);
		}

		static bool FileExists(std::string const& virtual_path)
		{
			ms_root_fs->FileExists(virtual_path);
		}

		static phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& virtual_path)
		{
			ms_root_fs->ReadFileSynchronous(virtual_path);
		}

        // -- Metadata retrival ---
		static Result<std::string> ResolveVirtualPath(std::string const& virtual_path)
		{
			return ms_root_fs->ResolveVirtualPath(virtual_path);
		}

		static Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path)
		{
			ms_root_fs->GetResourceDescriptorForAsync(virtual_path);
		}
		
		static Result<std::vector<std::string>> GetResourceDependencies(std::string const& virtual_path)
		{
			return ms_root_fs->GetResourceDependencies(virtual_path);
		}

		static Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path)
		{
			ms_root_fs->GetPlatformAttributes(virtual_path);
		}

		static Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path)
		{
			return ms_root_fs->GetUncompressedFileSize(virtual_path);
		}

		static bool Mount(std::string const& virtual_path, std::string const& physical_path)
		{
			return ms_root_fs->Mount(virtual_path, physical_path);
		}

		static bool Unmount(std::string const& virtual_path)
		{
			return ms_root_fs->Unmount(virtual_path);
		}

		static std::shared_ptr<IRootFileSystem>& GetFileSystem() { return ms_root_fs; }

	private:
		Vfs() = default;
		static std::shared_ptr<IRootFileSystem> ms_root_fs;
	};
}
