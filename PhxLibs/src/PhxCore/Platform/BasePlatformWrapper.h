#pragma once

#include <PhxCore/Memory/MemorySystem.h>
#include <PhxCore/EnumUtils.h>

namespace phx::platform
{
    enum class FileAttributes : uint32_t 
    {
        None = 0x00000000,
        ReadOnly = 0x00000001, // FILE_ATTRIBUTE_READONLY
        Hidden = 0x00000002, // FILE_ATTRIBUTE_HIDDEN
        System = 0x00000004, // FILE_ATTRIBUTE_SYSTEM
        Directory = 0x00000010, // FILE_ATTRIBUTE_DIRECTORY
        Archive = 0x00000020, // FILE_ATTRIBUTE_ARCHIVE
        Device = 0x00000040, // FILE_ATTRIBUTE_DEVICE
        Normal = 0x00000080, // FILE_ATTRIBUTE_NORMAL
        Temporary = 0x00000100, // FILE_ATTRIBUTE_TEMPORARY
        SparseFile = 0x00000200, // FILE_ATTRIBUTE_SPARSE_FILE
        ReparsePoint = 0x00000400, // FILE_ATTRIBUTE_REPARSE_POINT
        Compressed = 0x00000800, // FILE_ATTRIBUTE_COMPRESSED
        Offline = 0x00001000, // FILE_ATTRIBUTE_OFFLINE
        NotContentIndexed = 0x00002000, // FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
        Encrypted = 0x00004000, // FILE_ATTRIBUTE_ENCRYPTED
        IntegrityStream = 0x00008000, // FILE_ATTRIBUTE_INTEGRITY_STREAM
        Virtual = 0x00010000, // FILE_ATTRIBUTE_VIRTUAL
        NoScrubData = 0x00020000, // FILE_ATTRIBUTE_NO_SCRUB_DATA
        RecallOnOpen = 0x00040000, // FILE_ATTRIBUTE_RECALL_ON_OPEN (Same as EA)
        Pinned = 0x00080000, // FILE_ATTRIBUTE_PINNED
        Unpinned = 0x00100000, // FILE_ATTRIBUTE_UNPINNED
        RecallOnDataAccess = 0x00400000, // FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS

        Invalid = 0xFFFFFFFF  // Corresponds to INVALID_FILE_ATTRIBUTES
    };

    PHX_ENUM_CLASS_FLAGS(FileAttributes)

    template<class TDerived>
    class BasePlatformWrapper
    {
    public:

        void* VirtualMemReserve(size_t reserveSize)
        {
            return static_cast<TDerived*>(this)->PlatformVirtualMemReserve(reserveSize);
        }

        template<typename T, size_t _PageSize = 1>
        T* VirtualMemReserveTyped(size_t numEntries)
        {
            void* alloc = VirtualMemReserve(AlignUp(numEntries * sizeof(T), _PageSize));
            return static_cast<T*>(alloc);
        }

        void VirtualMemCommit(void* ptr, size_t commitSize)
        {
            return static_cast<TDerived*>(this)->PlatformVirtualMemCommit(ptr, commitSize);
        }

        bool VirtualMemFree(void* ptr)
        {
            return static_cast<TDerived*>(this)->PlatformVirtualMemFree(ptr);
        }

        FileAttributes GetFileAttr(std::string const& path)
        {
            return static_cast<TDerived*>(this)->PlatformGetFileAttributes(path);
        }
    protected:
        BasePlatformWrapper() = default;
        ~BasePlatformWrapper() = default;
    };

}
