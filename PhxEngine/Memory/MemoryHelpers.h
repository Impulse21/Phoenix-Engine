#pragma once

#include <PhxEngine/Memory/Memory.h>

// This need to be rethough out
#if false
namespace phx
{
    class FrameAllocator;

    template<typename T>
    using FramePtr = T*;
}

namespace phx::Memory
{
    template <typename T, typename... Args>
    [[nodiscard]] T* New(IAllocator& allocator, Args&&... args)
    {
        void* ptr = allocator.Alloc(sizeof(T), alignof(T));
        return ::new(ptr) T(std::forward<Args>(args)...);
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

    template<typename T>
    void DeleteArray(IHeapAllocator* alloc, T* ptr, usize count)
    {
        if (!ptr) return;
        for (usize i = 0; i < count; i++)
            ptr[i].~T();
        alloc->Free(ptr);
    }

    template<typename T>
    concept FrameSafe = std::is_trivially_destructible_v<T> &&
        std::is_trivially_copyable_v<T>;

    template<FrameSafe T, typename... Args>
    [[nodiscard]] FramePtr<T> FrameNew(FrameAllocator& alloc, Args&&... args)
    {
        void* ptr = alloc.Alloc(sizeof(T), alignof(T));
        return ::new(ptr) T(std::forward<Args>(args)...);
    }

    template<FrameSafe T>
    [[nodiscard]] FramePtr<T> FrameNewArray(FrameAllocator& alloc, usize count)
    {
        void* ptr = alloc.Alloc(sizeof(T) * count, alignof(T));
        return ::new(ptr) T[count];
    }
}

#define phx_new(allocator, Type, ...)               phx::Memory::New<Type>(allocator, ##__VA_ARGS__)
#define phx_frame_new(Type, ...)                    phx::Memory::FrameNew<Type>(Memory::g_Frame, ##__VA_ARGS__)


#define phx_new_array(allocator, Type, count)       phx::Memory::NewArray<Type>(allocator, count)
#define phx_frame_new_array(Type, count)            phx::Memory::FrameNewArray<Type>(Memory::g_Frame, count)

#define phx_delete(allocator, ptr)                  phx::Memory::Delete(allocator, ptr)
#define phx_delete_array(alloc, ptr, count)         phx::Memory::DeleteArray(alloc, ptr, count)

#endif