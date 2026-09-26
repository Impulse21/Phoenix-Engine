#include "ResourceCache.h"

#include <PhxEngine/VFS/VFS.h>

using namespace phx;

bool phx::resources::IsCookedFileUpToDate(const char* virtual_path, u32 magic, u16 version, u64 source_hash)
{
    VfsFileHandle handle = VFS::Open(virtual_path, platform::FileMode::Read | platform::FileMode::Binary);
    if (!handle.IsValid())
        return false;

    ResourceFileHeader header{};
    const usize read = VFS::Read(handle, &header, sizeof(header));
    VFS::Close(handle);

    return read == sizeof(header)
        && header.magic == magic
        && header.version == version
        && header.source_content_hash == source_hash;
}
