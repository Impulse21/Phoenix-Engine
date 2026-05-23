#pragma once

#include "IHeapAllocator.h"

namespace phx::Memory
{
    template <typename T, typename... Args>
    [[nodiscard]] T* New(IHeapAllocator& allocator, Args&&... args)
    {
        void* ptr = allocator.Alloc(sizeof(T), alignof(T));
        return ::new(ptr) T(std::forward<Args>(args)...);
    }

    template <typename T>
    void Delete(IHeapAllocator& allocator, T* ptr)
    {
        if (!ptr)
            return;
        
        ptr->~T();
        allocator.Free(ptr);
    }

    template <typename T>
    [[nodiscard]] T* AllocArray(IHeapAllocator& allocator, usize count)
    {
        void* ptr = allocator.Alloc(sizeof(T) * count, alignof(T));
        return ::new(ptr) T[count];
    }
}

#define phx_new(allocator, Type, ...)           phx::Memory::New<Type>(allocator, ##__VA_ARGS__)
#define phx_delete(allocator, ptr)              phx::Memory::Delete(allocator, ptr)
#define phx_alloc_array(allocator, Type, count) phx::Memory::AllocArray<Type>(allocator, count)