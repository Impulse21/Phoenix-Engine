#pragma once

#include <string>
#include <memory>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/Platform/Platform.h>

namespace phx
{
    // --- Enums ---
    enum class AsyncDataSourceType
    {
        Unknown_Or_Error,
        OS_File,
        Pak_Entry,
        Embedded,
    };

    enum class CompressionMethod
    {
        None,
        GDeflate
    };

    struct CompressionInfo
    {
        CompressionMethod method = CompressionMethod::None;
        uint64_t decompressed_size = 0; // If method != NONE, this is the target size after decompression
    };

    struct AsyncResourceDescriptor
    {
        AsyncDataSourceType type = AsyncDataSourceType::Unknown_Or_Error;

        std::string os_path_or_pak_path;
        std::string virtual_path;

        uint64_t offset_in_pak = 0;
        uint64_t length_of_resource = 0;
        const char* memory_buffer_ptr = nullptr; // For embedded resources

        CompressionInfo compression_info;

        bool IsValid() const
        {
            return type != AsyncDataSourceType::Unknown_Or_Error && !os_path_or_pak_path.empty();
        }
    };

    class IReader
    {
    public:
        virtual size_t Read(void *buffer, size_t size) = 0;
        virtual bool Seek(int64_t offset, FileSeekOrigin origin) = 0;

        virtual ~IReader() = default;
    };

    class IWriter
    {
    public:
        virtual size_t Write(const void *buffer, size_t size) = 0;

        virtual ~IWriter() = default;
    };

    class IFile : public IReader, public IWriter 
    {
    public:
        virtual size_t GetSize() = 0;
        virtual void Close() = 0;
        virtual AsyncDataSourceType GetSourceType() const = 0;

        virtual ~IFile() = default;
    };

    using FilePtr = std::unique_ptr<IFile, std::function<void(IFile*)>>;
	
    class IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		virtual bool FileExists(const std::string& name) = 0;
		virtual bool FolderExists(const std::string& name) = 0;

        virtual FilePtr Open(const std::string& path, FileMode file_mode) = 0;
        virtual phx::Result<PlatformFileHandle> OpenRaw(const std::string& path, FileMode file_mode) = 0;

		virtual bool WriteFile(const std::string& name, phx::Span<char> Data) = 0;
		virtual phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& path) const = 0;

    public:
		virtual Result<uint64_t> GetUncompressedFileSize(const std::string& path) const = 0;

        // -- This seems leaky
		virtual Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& path) const = 0;
		virtual Result<std::vector<std::string>> GetResourceDependencies(std::string const& path) const = 0;

		virtual Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& path) const = 0;
	};

	class IRootFileSystem : public IFileSystem
	{
	public:
		virtual ~IRootFileSystem() = default;

		virtual Result<std::string> ResolveVirtualPath(std::string const& path) const = 0;
		virtual bool Mount(const std::string& virtual_path, std::shared_ptr<IFileSystem> fs) = 0;
		virtual bool Mount(const std::string& virtual_path, std::string const& physical_path) = 0;
		virtual bool Unmount(const std::string& virtual_path) = 0;
	};
}