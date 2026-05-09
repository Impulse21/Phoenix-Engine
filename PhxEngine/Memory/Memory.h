#pragma once

namespace phx
{
    namespace Memory
    {
        enum class ArenaType { Virtual, System };

        struct MemoryDesc
        {
            ArenaType arena_type            = ArenaType::Virtual;

            size_t    main_reserve_bytes    = 32_GB;
            size_t    main_commit_bytes     = 64_MB;
        };

        [[nodiscard]] constexpr size_t PhxBytesToKB(size_t bytes) noexcept { return bytes >> 10; }
        [[nodiscard]] constexpr size_t PhxBytesToMB(size_t bytes) noexcept { return bytes >> 20; }
        [[nodiscard]] constexpr size_t PhxBytesToGB(size_t bytes) noexcept { return bytes >> 30; }
        
        [[nodiscard]] constexpr size_t PhxKB(size_t size) noexcept { return size << 10; }
        [[nodiscard]] constexpr size_t PhxMB(size_t size) noexcept { return size << 20; }
        [[nodiscard]] constexpr size_t PhxGB(size_t size) noexcept { return size << 30; }

        constexpr size_t operator"" _KB(unsigned long long bytes) { return bytes << 10; }
        constexpr size_t operator"" _MB(unsigned long long bytes) { return bytes << 20; }
        constexpr size_t operator"" _GB(unsigned long long bytes) { return bytes << 30; }

        
    } // namespace Memory
}