#pragma once

#include <PhxCore/IO/IFileSystems.h>
#include <PhxCore/Base.h>
#include <PhxCore/Memory/Allocators.h>

#include <string>
#include <memory>

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
		// Depericated
		std::string physical_path_normalized; // OS path to the directory or the PAK file
		std::shared_ptr<IFileSystem> fs;
		std::string virtual_prefix_normalized;

		// Add pak support

		MountPointInfo(Type t, std::string phys, std::string virt)
			: type(t)
			, physical_path_normalized(std::move(phys))
			, virtual_prefix_normalized(std::move(virt))
		{
		}	
		
		MountPointInfo(Type t, std::shared_ptr<IFileSystem> fs, std::string virt)
			: type(t)
			, fs(std::move(fs))
			, virtual_prefix_normalized(std::move(virt))
		{
		}
	};

	class PlatformFileSystem : public IFileSystem
    {
    public:
		bool FileExists(const std::string& name) override;
		bool FolderExists(const std::string& name) override;

        FilePtr Open(const std::string& path, FileMode file_mode) override;
        phx::Result<PlatformFileHandle> OpenRaw(const std::string& path, FileMode file_mode) override;

		bool WriteFile(const std::string& name, phx::Span<char> Data) override;
		phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& path) const override;

    public:
		Result<uint64_t> GetUncompressedFileSize(const std::string& path) const override;

        // -- This seems leaky
		Result<phx::AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& path) const override;
		Result<std::vector<std::string>> GetResourceDependencies(std::string const& path) const override;

		Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& path) const override;

	private:
		std::string NormalizeVirtualPath(const std::string& path) const;
		std::string NormalizePhysicalPath(const std::string& path) const; // For OS specific normalization

	private:
		TypedPoolAllocator<VfsFileBlock, 64> m_file_pool;
    };

    class RelativeFileSystem : public IFileSystem
    {
    public:
        RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::string& base_patth);

        [[nodiscard]] const std::string& GetBasePath() const { return m_base_path; }

		// -- Interface impl ---
    public:
		bool FileExists(const std::string& name) override;
		bool FolderExists(const std::string& name) override;

        FilePtr Open(const std::string& path, FileMode file_mode) override;
        phx::Result<PlatformFileHandle> OpenRaw(const std::string& path, FileMode file_mode) override;

		bool WriteFile(const std::string& name, phx::Span<char> Data) override;
		phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& path) const override;

    public:
		Result<uint64_t> GetUncompressedFileSize(const std::string& path) const override;

        // -- This seems leaky
		Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& path) const override;
		Result<std::vector<std::string>> GetResourceDependencies(std::string const& path) const override;

		Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& path) const override;

    private:
        std::shared_ptr<IFileSystem> m_underlyingFS;
        std::string m_base_path;
    };

    class RootFileSystem : public IRootFileSystem
    {
	public:
		RootFileSystem() = default;
		~RootFileSystem() = default;
		
    public:
		bool Mount(const std::string& virtual_path, std::shared_ptr<IFileSystem> fs) override;
		bool Mount(const std::string& virtual_path, const std::string& physical_path) override;
		bool Unmount(std::string const& virtual_path) override;

		bool FileExists(const std::string& virtual_path) override;
		bool FolderExists(const std::string& virtual_path) override;

        FilePtr Open(const std::string& virtual_path, FileMode file_mode) override;
        phx::Result<PlatformFileHandle> OpenRaw(const std::string& virtual_path, FileMode file_mode) override;

		bool WriteFile(const std::string& name, phx::Span<char> Data) override;
		phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& virtual_path) const override;

    public:
		Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path) const override;
		Result<std::string> ResolveVirtualPath(std::string const& virtual_path) const override;

        // -- This seems leaky
		Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path) const override;
		Result<std::vector<std::string>> GetResourceDependencies(std::string const& virtual_path) const override;

		Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path) const override;

    private:
        bool FindMountPoint(const std::string& virtual_path, std::string& physical_path, const MountPointInfo** mount_point) const;

    private:
        std::vector<std::pair<std::string, std::shared_ptr<IFileSystem>>> m_mountPoints;
		std::vector<MountPointInfo> m_mount_points;
    };
}

