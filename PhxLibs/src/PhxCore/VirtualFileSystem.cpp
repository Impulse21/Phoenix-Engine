#include "PhxCore/PhxCore_pch.h"
#include "VirtualFileSystem.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/Platform//PlatformWrapper.h>

using namespace phx;

bool VirtualFileSystem::Mount(std::string const& virtual_path, std::string const& physical_path)
{
    std::string norm_virtual_prefix = NormalizeVirtualPath(virtual_path);
    std::string norm_physical_path = NormalizePhysicalPath(physical_path);

    // Ensure virtual mount prefix ends with a separator for consistent matching
    if (!norm_virtual_prefix.empty() && norm_virtual_prefix.back() != '/')
        norm_virtual_prefix += '/';


    if (physical_path == "embedded://")
    {
        m_mount_points.emplace_back(
            MountPointInfo::Type::Embedded,
            norm_physical_path,
            norm_virtual_prefix);
    }
    else
    {
        // Platform-specific OS call to get file size and check if it's a file (not dir)
        Result<platform::PlatformFileAttributes> file_attributes = phx::Platform::Get().GetFileAttr(norm_physical_path);

        if (!file_attributes)
        {
            PHX_CORE_INFO("Physical path for mount '{0}' doesn't exist");
            return false;
        }

        if (file_attributes.GetValue().type == platform::PlatformFileType::Directory)
        {
            m_mount_points.emplace_back(
                MountPointInfo::Type::Directory,
                norm_physical_path,
                norm_virtual_prefix);
        }
        else
        {
            PHX_CORE_WARN("Pak File's are not supported yet");
            return true;
        }
    }

    // Sort mount points to ensure longest prefix is matched first
    std::sort(m_mount_points.begin(), m_mount_points.end(),
        [](const MountPointInfo& a, const MountPointInfo& b) {
            return a.virtual_prefix_normalized.length() > b.virtual_prefix_normalized.length();
        });

    // Log: Successfully mounted virtual_mount_point_str -> physical_path_str
    PHX_CORE_INFO("Successfully mounted '{0}' to '{1}'", norm_virtual_prefix.c_str(), norm_physical_path.c_str());
    return true;
}

bool VirtualFileSystem::Unmount(std::string const& virtual_path)
{
    std::string norm_virtual_prefix = NormalizeVirtualPath(virtual_path);
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

Result<std::string> phx::VirtualFileSystem::ResolveVirtualToPhysicalPath(std::string const& virtual_path) const
{
    std::string norm_virtual_path = NormalizeVirtualPath(virtual_path);
    const MountPointInfo* best_match = nullptr;
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
        PHX_CORE_ERROR("No mount point found for virtual path: {0}", norm_virtual_path.c_str());
        return phx::make_unexpected(~0ull);
    }

    std::string internal_path_segment = norm_virtual_path.substr(best_match->virtual_prefix_normalized.length());
    return JoinPaths(best_match->physical_path_normalized, internal_path_segment);
}

phx::Result<AsyncResourceDescriptor> VirtualFileSystem::GetResourceDescriptorForAsync(std::string const& virtual_path) const
{
    std::string norm_virtual_path = NormalizeVirtualPath(virtual_path);
    const MountPointInfo* best_match = nullptr;
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
        PHX_CORE_ERROR("No mount point found for virtual path: {0}", norm_virtual_path.c_str());
        return phx::make_unexpected(~0ull);
    }

    std::string internal_path_segment = norm_virtual_path.substr(best_match->virtual_prefix_normalized.length());
	std::string physical_path = JoinPaths(best_match->physical_path_normalized, internal_path_segment);

	if (best_match->type == MountPointInfo::Type::Embedded)
	{
        Result<Span<char>> embedded_res = phx::Platform::Get().GetEmbeddedResource(internal_path_segment);
        if (!embedded_res)
        {
            PHX_CORE_ERROR("Embedded Resource not found: {0}", physical_path.c_str());
            return make_unexpected(~0ull);
        }

        return AsyncResourceDescriptor{
            .type = AsyncDataSourceType::Embedded,
            .os_path_or_pak_path = physical_path,
            .virtual_path = norm_virtual_path,
            .offset_in_pak = 0,
            .length_of_resource = embedded_res->Size(),
            .memory_buffer_ptr = embedded_res->begin(),
            .compression_info = {.method = CompressionMethod::None }
        };
	}
    else if (best_match->type == MountPointInfo::Type::Directory) 
    {
        // Platform-specific OS call to get file size and check if it's a file (not dir)
        Result<platform::PlatformFileAttributes> file_attributes = phx::Platform::Get().GetFileAttr(physical_path);

        if (!file_attributes)
        {
            PHX_CORE_ERROR("Loose file not found or access error: {0}", physical_path.c_str());
            return make_unexpected(~0ull);
        }

        return AsyncResourceDescriptor{
            .type = AsyncDataSourceType::OS_File,
            .os_path_or_pak_path = physical_path,
            .virtual_path = norm_virtual_path,
            .offset_in_pak = 0,
            .length_of_resource = file_attributes->size,
            .compression_info = {.method = CompressionMethod::None }
        };
	}

	// handle back file
	PHX_CORE_ERROR("Internal VFS Error: PAK info not loaded for mount point {0}", best_match->virtual_prefix_normalized.c_str());
    return phx::make_unexpected(~0ull);
}

phx::Result<std::vector<std::string>> VirtualFileSystem::GetResourceDependencies(std::string const& /*virtual_path*/) const
{
    // TODO:
    return Result<std::vector<std::string>>();
}

phx::Result<phx::platform::PlatformFileAttributes> VirtualFileSystem::GetPlatformAttributes(std::string const& virtual_path) const
{
    std::string norm_virtual_path = NormalizeVirtualPath(virtual_path);
    const MountPointInfo* best_match = nullptr;
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
        PHX_CORE_ERROR("No mount point found for virtual path: {0}", norm_virtual_path.c_str());
        return phx::make_unexpected(~0ull);
    }

    // Platform-specific OS call to get file size and check if it's a file (not dir)
    std::string internal_path_segment = norm_virtual_path.substr(best_match->virtual_prefix_normalized.length());
    std::string physical_path = JoinPaths(best_match->physical_path_normalized, internal_path_segment);
    return phx::Platform::Get().GetFileAttr(physical_path);
}

bool VirtualFileSystem::Exists(std::string const& virtual_path)
{
    Result<platform::PlatformFileAttributes> file_attributes = GetPlatformAttributes(virtual_path);
    if (file_attributes.HasError())
        return false;

    return file_attributes->type == platform::PlatformFileType::File || file_attributes->type == platform::PlatformFileType::Directory;
}

phx::Result<uint64_t> VirtualFileSystem::GetUncompressedFileSize(const std::string& virtual_path) const
{
    phx::Result<AsyncResourceDescriptor> descriptor = GetResourceDescriptorForAsync(virtual_path);
    if (descriptor.HasError())
        return make_unexpected(~0ull);

    if (descriptor->compression_info.method == CompressionMethod::None)
        return descriptor->length_of_resource;

    return descriptor->compression_info.decompressed_size;
}

phx::Result<std::unique_ptr<phx::IBlob>> VirtualFileSystem::ReadFileSynchronous(const std::string& /*virtual_path*/) const
{
    PHX_CORE_ERROR("Not Implementated yet (VirtualFileSystem::ReadFileSynchronous");
    return make_unexpected(~0ull);;
}

std::string VirtualFileSystem::NormalizeVirtualPath(const std::string& path) const
{
    std::string temp = path;
    std::replace(temp.begin(), temp.end(), '\\', '/');

    return temp;
}

std::string VirtualFileSystem::NormalizePhysicalPath(const std::string& path) const
{
    std::string temp = path;
    std::replace(temp.begin(), temp.end(), '\\', '/');

    return temp;
}
