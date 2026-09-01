#include "VFS.h"

#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Pool.h>

#include <memory>
#include <cstring>

using namespace phx;
using namespace phx::VFS;

PHX_CVAR_INT(vfs_max_mounts,    16,  "Maximum simultaneous VFS mount points");
PHX_CVAR_INT(vfs_max_open_files, 64, "Maximum simultaneously open VFS files");

// ── Open file state ───────────────────────────────────────────────────────────
namespace phx
{
    struct VfsFileImpl
    {
        platform::PlatformFileHandle os_handle   = {};
        platform::FileMode           mode        = platform::FileMode::Read;
        u64                          base_offset = 0;   // 0 for loose files
        u64                          size        = 0;
        u64                          cursor      = 0;   // relative to base_offset
    };
}

namespace
{
    constexpr Log::Channel k_log = { "VFS" };

    // ── Pak layout ────────────────────────────────────────────────────────────
    // Written by the asset compiler. Entries are sorted by path_hash so lookup
    // is a binary search with no allocation.
    struct PakEntry
    {
        u64         path_hash;
        u64         offset;
        u64         size;
        u64         decompressed_size;
        Compression compression;
    };

    struct MountPoint
    {
        char        virtual_prefix[k_max_path] = {};
        u32         prefix_len                 = 0;
        char        physical_path[k_max_path]  = {};
        MountKind   kind                       = MountKind::Directory;

        PakEntry*   entries                    = nullptr;  // pak only
        u32         entry_count                = 0;
    };

    std::unique_ptr<MountPoint[]> s_mounts     = nullptr;
    u32    s_mount_count = 0;
    u32    s_mount_capacity = 0;

    Pool<VfsFile, VfsFileImpl> s_files;

    // ── Path helpers ──────────────────────────────────────────────────────────

    void NormalizeInto(char* dst, usize dst_size, const char* src)
    {
        usize i = 0;
        for (; src[i] && i < dst_size - 1; i++)
            dst[i] = (src[i] == '\\') ? '/' : src[i];
        dst[i] = '\0';
    }

    void JoinInto(char* dst, usize dst_size, const char* a, const char* b)
    {
        usize len = 0;
        while (a[len] && len < dst_size - 1)
        {
            dst[len] = a[len];
            len++;
        }

        if (len > 0 && dst[len - 1] != '/' && len < dst_size - 1)
            dst[len++] = '/';

        while (*b == '/')
            b++;

        while (*b && len < dst_size - 1)
            dst[len++] = *b++;

        dst[len] = '\0';
    }

    // FNV-1a — matches whatever the asset compiler uses for pak path hashing
    u64 HashPath(const char* path)
    {
        u64 hash = 14695981039346656037ull;
        for (const char* c = path; *c; c++)
        {
            char lower = (*c >= 'A' && *c <= 'Z') ? (*c + 32) : *c;
            hash ^= (u64)lower;
            hash *= 1099511628211ull;
        }
        return hash;
    }

    // ── Mount resolution ──────────────────────────────────────────────────────
    // Mounts are kept sorted by prefix length descending, so the first prefix
    // match is the longest one.
    const MountPoint* FindMount(const char* normalized_path, const char** out_relative)
    {
        for (u32 i = 0; i < s_mount_count; i++)
        {
            const MountPoint& m = s_mounts[i];
            if (strncmp(normalized_path, m.virtual_prefix, m.prefix_len) != 0)
                continue;

            const char* rel = normalized_path + m.prefix_len;
            while (*rel == '/')
                rel++;

            *out_relative = rel;
            return &m;
        }
        return nullptr;
    }

    void SortMountsByPrefixLength()
    {
        for (u32 i = 1; i < s_mount_count; i++)
        {
            MountPoint key = s_mounts[i];
            i32 j = (i32)i - 1;
            while (j >= 0 && s_mounts[j].prefix_len < key.prefix_len)
            {
                s_mounts[j + 1] = s_mounts[j];
                j--;
            }
            s_mounts[j + 1] = key;
        }
    }

    const PakEntry* FindPakEntry(const MountPoint& mount, const char* relative_path)
    {
        const u64 hash = HashPath(relative_path);

        u32 lo = 0;
        u32 hi = mount.entry_count;
        while (lo < hi)
        {
            u32 mid = lo + (hi - lo) / 2;
            if (mount.entries[mid].path_hash < hash)
            {
                lo = mid + 1;
            }
            else if (mount.entries[mid].path_hash > hash)
            {
                hi = mid;
            }
            else
            {
                return &mount.entries[mid];
            }
        }

        return nullptr;
    }
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

bool phx::VFS::Initialize()
{
    s_mount_capacity = (u32)CVar_vfs_max_mounts.Get();
    s_mounts         = std::make_unique<MountPoint[]>(s_mount_capacity);
    s_mount_count    = 0;

    s_files.Initialize((u32)CVar_vfs_max_open_files.Get());

    PHX_LOG_INFO(k_log, "Initialized — {} mount slots, {} file slots",
        s_mount_capacity, CVar_vfs_max_open_files.Get());

    return true;
}

void phx::VFS::Shutdown()
{
    s_mounts.reset();
    s_mount_count = 0;

    s_files.Shutdown();

    PHX_LOG_INFO(k_log, "Shutdown complete");
}

// ── Mounting ──────────────────────────────────────────────────────────────────

bool phx::VFS::Mount(const char* virtual_prefix, const char* physical_path)
{
    PHX_ASSERT(s_mount_count < s_mount_capacity);

    MountPoint& m = s_mounts[s_mount_count];
    NormalizeInto(m.virtual_prefix, k_max_path, virtual_prefix);
    NormalizeInto(m.physical_path,  k_max_path, physical_path);

    m.prefix_len = (u32)strlen(m.virtual_prefix);
    while (m.prefix_len > 0 && m.virtual_prefix[m.prefix_len - 1] == '/')
        m.virtual_prefix[--m.prefix_len] = '\0';

    m.kind        = MountKind::Directory;
    m.entries     = nullptr;
    m.entry_count = 0;

    s_mount_count++;
    SortMountsByPrefixLength();

    PHX_LOG_INFO(k_log, "Mounted '{}' -> '{}'", m.virtual_prefix, m.physical_path);
    return true;
}

bool phx::VFS::MountPak(const char* /*virtual_prefix*/, const char* pak_path)
{
    PHX_ASSERT(s_mount_count < s_mount_capacity);

    // TODO: read pak header + entry table into m.entries, sorted by path_hash.
    // The entry table is the only pak-specific work — everything downstream
    // treats a pak entry as a byte range in an OS file.
    PHX_LOG_WARN(k_log, "MountPak not implemented yet: '{}'", pak_path);
    return false;
}

bool phx::VFS::Unmount(const char* virtual_prefix)
{
    char normalized[k_max_path];
    NormalizeInto(normalized, k_max_path, virtual_prefix);

    for (u32 i = 0; i < s_mount_count; i++)
    {
        if (strcmp(s_mounts[i].virtual_prefix, normalized) != 0)
            continue;

        delete[] s_mounts[i].entries;

        s_mounts[i] = s_mounts[--s_mount_count];
        SortMountsByPrefixLength();

        PHX_LOG_INFO(k_log, "Unmounted '{}'", normalized);
        return true;
    }

    PHX_LOG_WARN(k_log, "Unmount failed — no such mount: '{}'", normalized);
    return false;
}

// ── Resolution ────────────────────────────────────────────────────────────────

bool phx::VFS::Resolve(const char* virtual_path, Location& out)
{
    char normalized[k_max_path];
    NormalizeInto(normalized, k_max_path, virtual_path);

    const char*       relative = nullptr;
    const MountPoint* mount    = FindMount(normalized, &relative);

    if (!mount)
    {
        PHX_LOG_WARN(k_log, "No mount point for '{}'", normalized);
        return false;
    }

    out.kind = mount->kind;

    if (mount->kind == MountKind::Directory)
    {
        JoinInto(out.os_path, k_max_path, mount->physical_path, relative);

        Result<platform::PlatformFileAttributes> attr = platform::GetFileAttr(out.os_path);
        if (!attr)
            return false;

        out.offset            = 0;
        out.size              = attr->size;
        out.decompressed_size = attr->size;
        out.compression       = Compression::None;
        return true;
    }

    const PakEntry* entry = FindPakEntry(*mount, relative);
    if (!entry)
        return false;

    NormalizeInto(out.os_path, k_max_path, mount->physical_path);
    out.offset            = entry->offset;
    out.size              = entry->size;
    out.decompressed_size = entry->decompressed_size;
    out.compression       = entry->compression;
    return true;
}

bool phx::VFS::Exists(const char* virtual_path)
{
    Location loc;
    return Resolve(virtual_path, loc);
}

// ── Whole-file IO ─────────────────────────────────────────────────────────────

Span<u8> phx::VFS::ReadFile(const char* virtual_path)
{
    VfsFileHandle handle = Open(virtual_path, platform::FileMode::Read);
    if (!handle.IsValid())
        return {};

    const u64 size = Size(handle);
    u8* buffer = new u8[size];

    const usize read = Read(handle, buffer, size);
    Close(handle);

    if (read != size)
    {
        PHX_LOG_ERROR(k_log, "Short read on '{}' — got {} of {} bytes",
            virtual_path, read, size);
        delete[] buffer;
        return {};
    }

    return Span<u8>(buffer, size);
}

bool phx::VFS::WriteFile(const char* virtual_path, Span<const u8> data)
{
    VfsFileHandle handle = Open(virtual_path, platform::FileMode::Write);
    if (!handle.IsValid())
        return false;

    const usize written = Write(handle, data.data(), data.Size());
    Close(handle);

    return written == data.Size();
}

// ── Streaming IO ──────────────────────────────────────────────────────────────

VfsFileHandle phx::VFS::Open(const char* virtual_path, platform::FileMode mode)
{
    Location loc;

    // Writes go to the mount's physical directory even if the file does not
    // exist yet, so Resolve failure is only fatal for reads.
    if (!Resolve(virtual_path, loc))
    {
        if (mode == platform::FileMode::Read)
            return {};

        char normalized[k_max_path];
        NormalizeInto(normalized, k_max_path, virtual_path);

        const char*       relative = nullptr;
        const MountPoint* mount    = FindMount(normalized, &relative);
        if (!mount || mount->kind != MountKind::Directory)
            return {};

        JoinInto(loc.os_path, k_max_path, mount->physical_path, relative);
        loc.offset = 0;
        loc.size   = 0;
    }

    if (loc.kind == MountKind::Pak && mode != platform::FileMode::Read)
    {
        PHX_LOG_ERROR(k_log, "Cannot write to pak-mounted path '{}'", virtual_path);
        return {};
    }

    Result<platform::PlatformFileHandle> os = platform::OpenFile(
        loc.os_path, platform::GetModeString(mode));

    if (os.HasError())
        return {};

    VfsFileHandle handle = s_files.Allocate();
    VfsFileImpl*  file   = s_files.Get(handle);

    file->os_handle   = os.GetValue();
    file->mode        = mode;
    file->base_offset = loc.offset;
    file->size        = loc.size;
    file->cursor      = 0;

    if (file->base_offset > 0)
        platform::SeekFile(file->os_handle, (i64)file->base_offset,
            platform::FileSeekOrigin::Begin);

    return handle;
}

void phx::VFS::Close(VfsFileHandle handle)
{
    VfsFileImpl* file = s_files.Get(handle);
    if (!file)
        return;

    platform::CloseFile(file->os_handle);
    s_files.Free(handle);
}

usize phx::VFS::Read(VfsFileHandle handle, void* dst, usize bytes)
{
    VfsFileImpl* file = s_files.Get(handle);
    if (!file)
        return 0;

    // Clamp to the file's byte range so pak entries cannot read past their end
    const u64 remaining = (file->cursor < file->size) ? (file->size - file->cursor) : 0;
    if (bytes > remaining)
        bytes = (usize)remaining;

    if (bytes == 0)
        return 0;

    const usize read = platform::ReadFile(file->os_handle, dst, bytes);
    file->cursor += read;
    return read;
}

usize phx::VFS::Write(VfsFileHandle handle, const void* src, usize bytes)
{
    VfsFileImpl* file = s_files.Get(handle);
    if (!file)
        return 0;

    platform::WriteFile(file->os_handle, (const char*)src, bytes);
    file->cursor += bytes;

    if (file->cursor > file->size)
        file->size = file->cursor;

    return bytes;
}

bool phx::VFS::Seek(VfsFileHandle handle, i64 offset, platform::FileSeekOrigin origin)
{
    VfsFileImpl* file = s_files.Get(handle);
    if (!file)
        return false;

    i64 target = 0;
    switch (origin)
    {
        case platform::FileSeekOrigin::Begin:   target = offset;                    break;
        case platform::FileSeekOrigin::Current: target = (i64)file->cursor + offset; break;
        case platform::FileSeekOrigin::End:     target = (i64)file->size + offset;   break;
    }

    if (target < 0)
        target = 0;

    if (target > (i64)file->size)
        target = (i64)file->size;

    file->cursor = (u64)target;

    return platform::SeekFile(file->os_handle,
        (i64)(file->base_offset + file->cursor),
        platform::FileSeekOrigin::Begin);
}

u64 phx::VFS::Tell(VfsFileHandle handle)
{
    VfsFileImpl* file = s_files.Get(handle);
    return file ? file->cursor : 0;
}

u64 phx::VFS::Size(VfsFileHandle handle)
{
    VfsFileImpl* file = s_files.Get(handle);
    return file ? file->size : 0;
}
