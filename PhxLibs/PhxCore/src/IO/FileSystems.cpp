#include "PhxCore_pch.h"
#include <PhxCore/IO/FileSystems.h>

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/Platform//Platform.h>

#include <fstream>

#include <filesystem>

using namespace phx;
namespace fs = std::filesystem;

namespace
{
    template<typename T, typename... Args>
    T* CreateFile(IAllocator& allocator, Args&&... args)
    {
        void* memory = allocator.Allocate(sizeof(T), alignof(T));
        if (!memory)
        {
            return nullptr;
        }

        return new (memory) T(std::forward<Args>(args)...);
    }

    void DestroyFile(IAllocator& allocator, IFile* object)
    {
        if (object)
        {
            object->Close();
            object->~IFile();

            allocator.Deallocate(object);
        }
    }

    // A helper to create a unique_ptr that knows how to talk to an IAllocator
    auto MakeUnique(IAllocator& alloc, IFile* ptr)
    {
        return std::unique_ptr<IFile, std::function<void(IFile*)>> (ptr, [&alloc](IFile* p) 
        {
            DestroyFile(alloc, p);
        });
    }
}


bool PlatformFileSystem::FileExists(const std::string& name)
{
    return phx::FileExists(name);
}

bool PlatformFileSystem::FolderExists(const std::string& name)
{
    return phx::DirectoryExists(name);
}

phx::Result<std::unique_ptr<phx::IBlob>> PlatformFileSystem::ReadFileSynchronous(const std::string& name) const
{
    // TODO: Use my platform layer which is newer.
    std::ifstream file(name, std::ios::binary);

    if (!file.is_open())
    {
        // file does not exist or is locked
        return Unexpected(ResultError::NotFound);
    }

    file.seekg(0, std::ios::end);
    uint64_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    {
        PHX_CORE_ERROR("File larger then size_t");
        return Unexpected(ResultError::Failure);
    }

    char* Data = static_cast<char*>(malloc(size));

    if (Data == nullptr)
    {
        PHX_CORE_ERROR("Out of memory");
        return Unexpected(ResultError::Failure);
    }

    file.read(Data, size);

    if (!file.good())
    {
        PHX_CORE_ERROR("Reading error");
        free(Data);
        return Unexpected(ResultError::Failure);
    }

    return std::make_unique<Blob>(Data, size);
}

bool PlatformFileSystem::WriteFile(const std::string& name, phx::Span<char> Data)
{
    // TODO: Use platofrm api
    std::ofstream file(name, std::ios::binary);

    if (!file.is_open())
    {
        PHX_CORE_ERROR("File does not exist or is locked");
        return false;
    }

    if (Data.Size() > 0)
    {
        file.write(Data.begin(), static_cast<std::streamsize>(Data.Size()));
    }

    if (!file.good())
    {
        PHX_CORE_ERROR("Failed to write file.");
        return false;
    }

    return true;
}

FilePtr PlatformFileSystem::Open(const std::string& path, FileMode file_mode)
{    
    const char* mode = Platform::GetModeString(file_mode);

    phx::Result<PlatformFileHandle> os_file_handle = Platform::OpenFile(path, mode);
    if (os_file_handle.HasError())
        return nullptr;

    Result<PlatformFileAttributes> file_attributes = phx::Platform::GetFileAttr(path);
    if (!file_attributes)
    {
        PHX_CORE_WARN("Loose file not found or access error: {0}", path.c_str());
        return nullptr;
    }

    OsFile* os_file = CreateFile<OsFile>(m_file_pool);
    if (!os_file) 
        return nullptr;

    os_file->mode = file_mode;
    os_file->os_handle = os_file_handle.GetValue();
    os_file->size = file_attributes->size;

    return MakeUnique(m_file_pool, os_file);
}

phx::Result<PlatformFileHandle> PlatformFileSystem::OpenRaw(const std::string& path, FileMode file_mode)
{
    const char* mode = Platform::GetModeString(file_mode);
    return Platform::OpenFile(path, mode);
}

Result<uint64_t> PlatformFileSystem::GetUncompressedFileSize(const std::string& path) const
{

    phx::Result<AsyncResourceDescriptor> descriptor = GetResourceDescriptorForAsync(path);
    if (descriptor.HasError())
        return Unexpected(ResultError::Failure);

    if (descriptor->compression_info.method == CompressionMethod::None)
        return descriptor->length_of_resource;

    return descriptor->compression_info.decompressed_size;
}

// -- This seems leaky
Result<phx::AsyncResourceDescriptor> PlatformFileSystem::GetResourceDescriptorForAsync(std::string const& path) const
{
    // Platform-specific OS call to get file size and check if it's a file (not dir)
    Result<PlatformFileAttributes> file_attributes = phx::Platform::GetFileAttr(path);

    if (!file_attributes)
    {
        PHX_CORE_WARN("Loose file not found or access error: {0}", path.c_str());
        return Unexpected(ResultError::Failure);
    }

    return AsyncResourceDescriptor{
        .type = AsyncDataSourceType::OS_File,
        .os_path_or_pak_path = path,
        .virtual_path = NormalizeVirtualPath(path), // TODO: We don't hav ethis here. Not sure how to adjust this
        .offset_in_pak = 0,
        .length_of_resource = file_attributes->size,
        .compression_info = {.method = CompressionMethod::None}};
}

Result<std::vector<std::string>> PlatformFileSystem::GetResourceDependencies(std::string const& /*path*/) const
{
    return std::vector<std::string>();
}

Result<PlatformFileAttributes> PlatformFileSystem::GetPlatformAttributes(std::string const& path) const
{
    return phx::Platform::GetFileAttr(path);
}

RelativeFileSystem::RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::string& base_path)
    : m_underlyingFS(std::move(fs))
    , m_base_path(base_path)
{
}

bool RelativeFileSystem::FileExists(const std::string& name)
{
    return m_underlyingFS->FileExists(JoinPaths(m_base_path, name));
}

bool RelativeFileSystem::FolderExists(const std::string& name)
{
    return m_underlyingFS->FolderExists(JoinPaths(m_base_path, name));
}

FilePtr RelativeFileSystem::Open(const std::string &path, FileMode file_mode)
{
    return m_underlyingFS->Open(JoinPaths(m_base_path, path), file_mode);
}

phx::Result<PlatformFileHandle> RelativeFileSystem::OpenRaw(const std::string &path, FileMode file_mode)
{
    return m_underlyingFS->OpenRaw(JoinPaths(m_base_path, path), file_mode);
}

bool RelativeFileSystem::WriteFile(const std::string &name, phx::Span<char> data)
{
    return m_underlyingFS->WriteFile(JoinPaths(m_base_path, name), data);
}

phx::Result<std::unique_ptr<phx::IBlob>> RelativeFileSystem::ReadFileSynchronous(const std::string &path) const
{
    return m_underlyingFS->ReadFileSynchronous(JoinPaths(m_base_path, path));
};

Result<uint64_t> RelativeFileSystem::GetUncompressedFileSize(const std::string &path) const
{
    return m_underlyingFS->GetUncompressedFileSize(JoinPaths(m_base_path, path));
}

Result<AsyncResourceDescriptor> RelativeFileSystem::GetResourceDescriptorForAsync(std::string const &path) const
{
    return m_underlyingFS->GetResourceDescriptorForAsync(JoinPaths(m_base_path, path));
}

Result<std::vector<std::string>> RelativeFileSystem::GetResourceDependencies(std::string const &path) const
{
    return m_underlyingFS->GetResourceDependencies(JoinPaths(m_base_path, path));
}

Result<PlatformFileAttributes> RelativeFileSystem::GetPlatformAttributes(std::string const &path) const
{
    return m_underlyingFS->GetPlatformAttributes(JoinPaths(m_base_path, path));
}

bool RootFileSystem::Mount(const std::string& virtual_path, std::shared_ptr<IFileSystem> fs)
{
    PHX_ASSERT(fs);
    std::string norm_virtual_prefix = NormalizePath(virtual_path);

    if (!norm_virtual_prefix.empty() && norm_virtual_prefix.back() != '/')
        norm_virtual_prefix += '/';

    std::string physical_path;
    if (this->FindMountPoint(virtual_path, physical_path, nullptr))
    {
        PHX_CORE_ERROR("Cannot mount a filesystem at {0}: there is another FS that includes this path", virtual_path.c_str());
        return false;
    }

    m_mount_points.emplace_back(
        MountPointInfo::Type::Directory,
        std::move(fs),
        norm_virtual_prefix);

    std::sort(m_mount_points.begin(), m_mount_points.end(),
        [](const MountPointInfo& a, const MountPointInfo& b) {
            return a.virtual_prefix_normalized.length() > b.virtual_prefix_normalized.length();
    });

    return true;
}

bool RootFileSystem::Mount(const std::string& virtual_path, const std::string& physical_path)
{
    std::string normalize_physical_path = NormalizePath(physical_path);

    if (normalize_physical_path == "embedded://")
    {
        PHX_ASSERT(false, "Not currently supported");
        return false;
    }

    auto base_file_system = std::make_shared<PlatformFileSystem>();
    auto relative_file_system = std::make_shared<RelativeFileSystem>(base_file_system, normalize_physical_path);
    const bool result = Mount(virtual_path, relative_file_system);
    if (result)
    {
        PHX_CORE_INFO("Successfully mounted '{0}' to '{1}'", virtual_path.c_str(), normalize_physical_path.c_str());
    }

    return result;
}

bool RootFileSystem::Unmount(std::string const& virtual_path)
{
    std::string norm_virtual_prefix = NormalizePath(virtual_path);
    if (!norm_virtual_prefix.empty() && norm_virtual_prefix.back() != '/')
        norm_virtual_prefix += '/';

    auto it = std::remove_if(
        m_mount_points.begin(), m_mount_points.end(),
        [&](const MountPointInfo& mp) { 
            return mp.virtual_prefix_normalized == norm_virtual_prefix; 
        });

    if (it != m_mount_points.end()) 
    {
        m_mount_points.erase(it, m_mount_points.end());
        PHX_CORE_INFO("Unmounted '{0}'", virtual_path.c_str());
        return true;
    }

    PHX_CORE_INFO("Mount point '{0}' not found for unmounting.", virtual_path.c_str());
    return false;
}

// TODO: I AM HERE
bool RootFileSystem::FileExists(const std::string& virtual_path)
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->FileExists(physical_path);
    }

    return false;
}

bool RootFileSystem::FolderExists(const std::string& virtual_path)
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;

    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->FileExists(physical_path);
    }

    return false;
}

FilePtr RootFileSystem::Open(const std::string& virtual_path, FileMode file_mode)
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->Open(physical_path, file_mode);
    }

    return nullptr;
}

phx::Result<PlatformFileHandle> RootFileSystem::OpenRaw(const std::string& virtual_path, FileMode file_mode)
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->OpenRaw(physical_path, file_mode);
    }

    return Unexpected(ResultError::Failure);
}


bool RootFileSystem::WriteFile(const std::string& virtual_path, phx::Span<char> data)
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->WriteFile(physical_path, data);
    }

    return false;
}

phx::Result<std::unique_ptr<phx::IBlob>> RootFileSystem::ReadFileSynchronous(const std::string& virtual_path) const
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->ReadFileSynchronous(physical_path);
    }

    return Unexpected(ResultError::Failure);
}

Result<uint64_t> RootFileSystem::GetUncompressedFileSize(const std::string& virtual_path) const
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->GetUncompressedFileSize(physical_path);
    }

    return Unexpected(ResultError::Failure);
}

Result<std::string> RootFileSystem::ResolveVirtualPath(std::string const& virtual_path) const
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return physical_path;
    }

    return Unexpected(ResultError::Failure);
}


Result<AsyncResourceDescriptor> RootFileSystem::GetResourceDescriptorForAsync(std::string const& virtual_path) const
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        Result<AsyncResourceDescriptor> descriptor = mount_point->fs->GetResourceDescriptorForAsync(physical_path);
        if (!descriptor.HasError())
        {
            descriptor->virtual_path = virtual_path;
        }

        return descriptor;
    }

    return Unexpected(ResultError::Failure);
};

Result<std::vector<std::string>> RootFileSystem::GetResourceDependencies(std::string const& virtual_path) const
{    
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->GetResourceDependencies(physical_path);
    }

    return Unexpected(ResultError::Failure);
}

Result<PlatformFileAttributes> RootFileSystem::GetPlatformAttributes(std::string const& virtual_path) const
{
    const MountPointInfo* mount_point = nullptr;
    std::string physical_path;
    
    if (FindMountPoint(virtual_path, physical_path, &mount_point))
    {
        return mount_point->fs->GetPlatformAttributes(physical_path);
    }

    return Unexpected(ResultError::Failure);
}


bool RootFileSystem::FindMountPoint(const std::string& virtual_path, std::string& physical_path, const MountPointInfo** mount_point) const
{
    std::string norm_virtual_path = NormalizePath(virtual_path);

    const MountPointInfo* best_match = *mount_point;
    for (const auto& mp : m_mount_points)
    {
        if (norm_virtual_path.rfind(mp.virtual_prefix_normalized, 0) == 0)
        {
            best_match = &mp;
            break; // Found longest prefix due to sort order
        }
    }

    if (!best_match)
    {
        PHX_CORE_WARN("No mount point found for virtual path: {0}", norm_virtual_path.c_str());
        return false;
    }

    std::string internal_path_segment = norm_virtual_path.substr(best_match->virtual_prefix_normalized.length());
	physical_path = JoinPaths(best_match->physical_path_normalized, internal_path_segment);

    return false;
}

OsFile::OsFile() {};
OsFile::~OsFile()
{
    Close();
}

size_t OsFile::Write(const void *buffer, size_t size)
{
    if (!os_handle.IsValid())
    {
        PHX_ASSERT(false, "invalid OS handle");
        return 0;
    }

    if (!EnumHasAnyFlags(FileMode::Write, mode))
    {
        PHX_ASSERT(false, "Attempting to write to readonly file");
        return 0;
    }

    Platform::WriteFile(os_handle, static_cast<const char*>(buffer), size);

    // TODO: Whouls get this back from the platform
    // to determine what was actaully written.
    return size;
}

size_t OsFile::Read(void *buffer, size_t size)
{
    if (!os_handle.IsValid())
    {
        PHX_ASSERT(false, "invalid OS handle");
        return 0;
    }

    if (!EnumHasAnyFlags(FileMode::Read, mode))
    {
        PHX_ASSERT(false, "Attempting to write to readonly file");
        return 0;
    }

    return Platform::ReadFile(os_handle, buffer, size);
}

bool OsFile::Seek(int64_t offset, FileSeekOrigin origin)
{
    if (!os_handle.IsValid())
    {
        PHX_ASSERT(false, "invalid OS handle");
        return 0;
    }

    return Platform::SeekFile(os_handle, offset, origin);
}

void OsFile::Close()
{
    if (!os_handle.IsValid())
        return;

    Platform::CloseFile(os_handle);
    os_handle = {};
}