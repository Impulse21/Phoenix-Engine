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

        
    } // namespace Memory
}