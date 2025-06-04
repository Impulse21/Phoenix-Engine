#include "PhxData/PhxData_pch.h"
#include "VirtualFileSystemImpl.h"

#include <PhxCore/Platform//PlatformWrapper.h>

using namespace phx::data;

bool VirtualFileSystemImpl::Mount(std::string const& virtual_path, std::string const& physical_path)
{
    std::string norm_virtual_prefix = NormalizeVirtualPath(virtual_path);
    std::string norm_physical_path = NormalizePhysicalPath(physical_path);

    // Ensure virtual mount prefix ends with a separator for consistent matching
    if (!norm_virtual_prefix.empty() && norm_virtual_prefix.back() != '/')
        norm_virtual_prefix += '/';

    phx::platform::FileAttributes fileAttributes = phx::Platform::Get().GetFileAttr(norm_physical_path);

    if (phx::EnumHasAnyFlags(phx::platform::FileAttributes::Invalid, fileAttributes))
    {
        PHX_CORE_INFO("Physical path for mount '{0}' doesn't exist");
        return false;
    }

    const bool is_directory_type = 
        phx::EnumHasAnyFlags(phx::platform::FileAttributes::Directory, fileAttributes);

    if (is_directory_type) 
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

    // Sort mount points to ensure longest prefix is matched first
    std::sort(m_mount_points.begin(), m_mount_points.end(),
        [](const MountPointInfo& a, const MountPointInfo& b) {
            return a.virtual_prefix_normalized.length() > b.virtual_prefix_normalized.length();
        });

    // Log: Successfully mounted virtual_mount_point_str -> physical_path_str
    PHX_CORE_INFO("Successfully mounted '{0}' to '{1}'", norm_virtual_prefix.c_str(), norm_virtual_prefix.c_str());
    return true;
}

bool VirtualFileSystemImpl::Unmount(std::string const& virtual_path)
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

Result<AsyncResourceDescriptor> VirtualFileSystemImpl::GetResourceDescriptorForAsync(std::string const& virtual_path)
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
        return "No mount point found for virtual path: " + norm_virtual_path;
    }

    AsyncResourceDescriptor desc;
    desc.virtual_path = norm_virtual_path;

    std::string internal_path_segment = norm_virtual_path.substr(best_match->virtual_prefix_normalized.length());

    if (best_match->type == MountPointInfo::Type::Directory) 
    {
        desc.type = AsyncDataSourceType::Os_File;
        desc.os_path_or_pak_path = JoinPaths(best_match->physical_path_normalized, internal_path_segment);
        desc.offset_in_pak = 0;
        desc.compression_info.method = CompressionMethod::None;

        // Platform-specific OS call to get file size and check if it's a file (not dir)
        phx::Platform::Get().GetFileAttr(desc.os_path_or_pak_path);

        WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
        std::wstring wide_os_path(desc.os_path_or_archive_path.begin(), desc.os_path_or_archive_path.end());
        if (GetFileAttributesExW(wide_os_path.c_str(), GetFileExInfoStandard, &fileAttributes)) {
            if (fileAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                return "Path resolves to a directory, not a file: " + desc.os_path_or_archive_path;
            }
            ULARGE_INTEGER fileSize;
            fileSize.HighPart = fileAttributes.nFileSizeHigh;
            fileSize.LowPart = fileAttributes.nFileSizeLow;
            desc.length_of_resource = fileSize.QuadPart;
            desc.compression_info.decompressed_size = desc.length_of_resource; // Same for uncompressed
        }
        else 
        {
            PHX_CORE_ERROR("Loose file not found or access error: {0}", desc.os_path_or_pak_path.c_str());
            return "Loose file not found or access error: " + desc.os_path_or_pak_path;
        }
    }
    else 
    { 
        PHX_CORE_ERROR("Internal VFS Error: PAK info not loaded for mount point {0}", best_match->virtual_prefix_normalized.c_str());
        return "Internal VFS Error: PAK info not loaded for mount point " + best_match->virtual_prefix_normalized;
    }

    if (!desc.IsValid()) 
    { 
        PHX_CORE_ERROR("Generated descriptor is invalid (e.g., zero length resource): {0}", virtual_path.c_str());
        return "Generated descriptor is invalid (e.g., zero length resource): " + virtual_path;
    }
    return desc;

}

std::string VirtualFileSystemImpl::NormalizeVirtualPath(const std::string& path) const
{
    std::string temp = path;
    std::replace(temp.begin(), temp.end(), '\\', '/');

    return temp;
}

std::string VirtualFileSystemImpl::NormalizePhysicalPath(const std::string& path) const
{
    std::string temp = path;
    std::replace(temp.begin(), temp.end(), '\\', '/');

    return temp;
}
