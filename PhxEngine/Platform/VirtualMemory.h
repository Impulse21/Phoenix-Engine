#pragma once

namespace phx::platform::VirtualMemory
{
    usize GetPageSize();
    usize GetAllocationGranularity();

    [[nodiscard]] void* Reserve(usize size);
    bool Commit(void* ptr, usize size);
    void Release(void* ptr, usize size);
}