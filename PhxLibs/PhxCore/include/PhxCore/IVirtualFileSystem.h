#pragma once

#include <string>
#include <memory>

#include <PhxCore/Platform/Platform.h>

namespace phx
{
	class IBlob;
}

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

	class IVirtualFileSystem
	{
	public:
		inline static IVirtualFileSystem* Ptr = nullptr;
	public:
		virtual ~IVirtualFileSystem() = default;

        // -- File API ---
    public:
        virtual FilePtr Open(const std::string& virtual_path, FileMode file_mode) = 0;
        virtual phx::Result<PlatformFileHandle> OpenRaw(const std::string& virtual_path, FileMode file_mode) = 0;

		virtual bool Exists(std::string const& virtual_path) = 0;
		virtual phx::Result<std::unique_ptr<phx::IBlob>> ReadFileSynchronous(const std::string& virtual_path) const = 0;
        
        // -- Metadata retrival ---
    public:
		virtual Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path) const = 0;
		virtual Result<std::string> ResolveVirtualToPhysicalPath(std::string const& virtual_path) const = 0;
		virtual Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path) const = 0;
		virtual Result<std::vector<std::string>> GetResourceDependencies(std::string const& virtual_path) const = 0;

		virtual Result<PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path) const = 0;

        // -- Mount Functions ---
    public:
		virtual bool Mount(std::string const& virtual_path, std::string const& physical_path) = 0;
		virtual bool Unmount(std::string const& virtual_path) = 0;
	};
}