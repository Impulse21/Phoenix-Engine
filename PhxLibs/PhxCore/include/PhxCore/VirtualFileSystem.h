#pragma once

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Base.h>
#include <PhxCore/Memory/Allocators.h>
namespace phx
{
	struct OsFile : public IFile
	{
		FileMode mode;
		PlatformFileHandle os_handle;
		size_t size;
		
		OsFile();
		~OsFile();

		// -- Write interface ---
        size_t Write(const void *buffer, size_t size) override;

		// -- Reader Interface ---
        size_t Read(void *buffer, size_t size) override;
        bool Seek(int64_t offset, FileSeekOrigin origin) override;

		// -- File Interface ---
        void Close() override;
        size_t GetSize() override { return size; }
		AsyncDataSourceType GetSourceType() const override { return AsyncDataSourceType::OS_File; }
	};

	constexpr size_t MAX_VFS_FILE_SIZE = sizeof(OsFile);
	constexpr size_t MAX_VFS_ALIGN = alignof(OsFile);

	struct ALIGN_TYPE(MAX_VFS_ALIGN) VfsFileBlock
	{
		char data[MAX_VFS_FILE_SIZE];
	};

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

	class VirtualFileSystem final : public IVirtualFileSystem
	{
	public:
		VirtualFileSystem() = default;
		~VirtualFileSystem() override = default;

        // -- File API ---
    public:
        phx::Result<FilePtr> Open(const std::string& virtual_path, FileMode file_mode) override;
        phx::Result<PlatformFileHandle> OpenRaw(const std::string& virtual_path, FileMode file_mode) override;

		bool Exists(std::string const& virtual_path) override;
		phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& virtual_path) const override;

        // -- Metadata retrival ---
    public:
		Result<std::string> ResolveVirtualToPhysicalPath(std::string const& virtual_path) const override;
		Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path) const override;
		Result<std::vector<std::string>> GetResourceDependencies(std::string const& virtual_path) const override;

		Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path) const override;
		Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path) const override;

        // -- Mount Functions ---
	public:
		bool Mount(std::string const& virtual_path, std::string const& physical_path) override;
		bool Unmount(std::string const& virtual_path) override;

	private:
		std::string NormalizeVirtualPath(const std::string& path) const;
		std::string NormalizePhysicalPath(const std::string& path) const; // For OS specific normalization

	private:
		std::vector<MountPointInfo> m_mount_points;
		TypedPoolAllocator<VfsFileBlock, 64> m_file_pool;
	};
}

