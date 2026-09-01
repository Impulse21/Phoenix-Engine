#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Platform/IO.h>

namespace phx
{
    struct VfsFile;
    using VfsFileHandle = Handle<VfsFile>;

    namespace VFS
    {
        constexpr u32 k_max_path = 260;

        enum class MountKind : u8
        {
            Directory,
            Pak,
        };

        enum class Compression : u8
        {
            None,
            GDeflate,
        };

        // Where a virtual path physically lives.
        // A pak entry is just a byte range inside an OS file, so both
        // mount kinds resolve to the same shape.
        struct Location
        {
            char        os_path[k_max_path] = {};
            u64         offset              = 0;   // 0 for loose files
            u64         size                = 0;   // bytes on disk
            u64         decompressed_size   = 0;   // == size when uncompressed
            Compression compression         = Compression::None;
            MountKind   kind                = MountKind::Directory;
        };

        bool Initialize();
        void Shutdown();

        // Mounting. Longest virtual prefix wins on resolve.
        bool Mount     (const char* virtual_prefix, const char* physical_path);
        bool MountPak  (const char* virtual_prefix, const char* pak_path);
        bool Unmount   (const char* virtual_prefix);

        // Resolution
        bool Exists    (const char* virtual_path);
        bool Resolve   (const char* virtual_path, Location& out);

        // Whole-file read. Caller owns the returned memory and must
        // release it with delete[]. Returns an empty span on failure.
        [[nodiscard]] Span<u8> ReadFile (const char* virtual_path);
        bool                   WriteFile(const char* virtual_path, Span<const u8> data);

        // Streaming. Reads are clamped to the file's byte range, so pak
        // entries behave identically to loose files.
        VfsFileHandle Open  (const char* virtual_path, platform::FileMode mode);
        void          Close (VfsFileHandle handle);
        usize         Read  (VfsFileHandle handle, void* dst, usize bytes);
        usize         Write (VfsFileHandle handle, const void* src, usize bytes);
        bool          Seek  (VfsFileHandle handle, i64 offset, platform::FileSeekOrigin origin);
        u64           Tell  (VfsFileHandle handle);
        u64           Size  (VfsFileHandle handle);
    }
}
