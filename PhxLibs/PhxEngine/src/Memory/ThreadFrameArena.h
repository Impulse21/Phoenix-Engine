#pragma once

#include <new>     // For std::align_val_t, std::bad_alloc
#include <PhxCore/Memory/IAllocator.h>

namespace phx
{
    // --- Thread Frame Arena Interface ---
    struct ThreadFrameArena final : public IAllocator
    {
        ThreadFrameArena() = default;
        ~ThreadFrameArena() = default;

        void Initialize(size_t reserveBytes, size_t initialCommitBytes);
        void Shutdown();

        [[nodiscard]] void* Allocate(size_t size, size_t alignment) override;
        [[nodiscard]] void* Allocate(size_t size, size_t alignment, const char*, int32_t) override
        {
            return Allocate(size, alignment);
        }

        void Deallocate(void* pointer) override;

        void Reset();
        bool IsAddressInRange(const void* ptr) const override;
        void* GetBaseAddress() const { return m_baseAddress; }
        size_t GetReservedSize() const { return m_reservedSize; }
        size_t GetCommitedSize() const { return m_commitedSize; }

        uint8_t* m_baseAddress = nullptr;
        size_t m_allocatedSize = 0;
        size_t m_reservedSize = 0;
        size_t m_commitedSize = 0;
        size_t m_pageSize = 0;
    };
}