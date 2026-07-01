#pragma once

#include "IHeapAllocator.h"

#include "FramePtr.h"

namespace phx
{
    class FrameAllocator;
}

namespace phx::Memory
{
    template <typename T, typename... Args>
    [[nodiscard]] T* New(IAllocator& allocator, Args&&... args)
    {
        void* ptr = allocator.Alloc(sizeof(T), alignof(T));
        return ::new(ptr) T(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    [[nodiscard]] FramePtr<T> FrameNew(FrameAllocator& allocator, Args&&... args)
    {
        T* ptr = New(allocator, std::forward<Args>(args)...);
        return FramePtr<T>(ptr);
    }

    template <typename T>
    void Delete(IAllocator& allocator, T* ptr)
    {
        if (!ptr)
            return;
        
        ptr->~T();
        allocator.Free(ptr);
    }

    template <typename T>
    [[nodiscard]] T* NewArray(IAllocator& allocator, usize count)
    {
        void* ptr = allocator.Alloc(sizeof(T) * count, alignof(T));
        return ::new(ptr) T[count];
    }

    template <typename T>
    [[nodiscard]] FramePtr<T> FrameNewArray(FrameAllocator& allocator, usize count)
    {
        T* ptr = NewArray(allocator, count);
        return FramePtr<T>(ptr);
    }
}

#define phx_new(allocator, Type, ...)           phx::Memory::New<Type>(allocator, ##__VA_ARGS__)
#define phx_frame_new(allocator, Type, ...)     phx::Memory::FrameNew<Type>(allocator, ##__VA_ARGS__)


#define phx_new_array(allocator, Type, count) phx::Memory::NewArray<Type>(allocator, count)
#define phx_frame_new_array(allocator, Type, count) phx::Memory::FrameNewArray<Type>(allocator, count)

#define phx_delete(allocator, ptr)              phx::Memory::Delete(allocator, ptr)