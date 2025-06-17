#pragma once

#include <PhxData/IVirtualFileSystem.h>

namespace phx::data
{

	// Internal structure for a mount point
	struct MountPointInfo 
	{
		enum class Type { Directory, Pak, Embedded, };
		Type type;
		std::string physical_path_normalized; // OS path to the directory or the PAK file
		std::string virtual_prefix_normalized;

		// Add pak support

		MountPointInfo(Type t, std::string phys, std::string virt)
			: type(t)
			, physical_path_normalized(std::move(phys))
			, virtual_prefix_normalized(std::move(virt))
		{
		}
	};

	class VirtualFileSystemImpl final : public IVirtualFileSystem
	{
	public:
		VirtualFileSystemImpl() = default;
		~VirtualFileSystemImpl() override = default;

	public:
		bool Mount(std::string const& virtual_path, std::string const& physical_path) override;
		bool Unmount(std::string const& virtual_path) override;

		Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path) const override;
		Result<std::vector<std::string>> GetResourceDependencies(std::string const& virtual_path) const override;

		virtual Result<platform::PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path) const override;

		bool Exists(std::string const& virtual_path) override;
		Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path) const override;
		phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& virtual_path) const override;

	private:
		std::string NormalizeVirtualPath(const std::string& path) const;
		std::string NormalizePhysicalPath(const std::string& path) const; // For OS specific normalization

	private:
		std::vector<MountPointInfo> m_mount_points;
	};
}

