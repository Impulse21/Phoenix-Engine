#pragma once

#include <PhxEngine/Memory/Memory.h>

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
}

#define phx_new(allocator, Type, ...)               phx::Memory::New<Type>(allocator, ##__VA_ARGS__)
#define phx_frame_new(Type, ...)                    phx::Memory::New<Type>(Memory::g_Frame, ##__VA_ARGS__)


#define phx_new_array(allocator, Type, count)       phx::Memory::NewArray<Type>(allocator, count)
#define phx_frame_new_array(Type, count)            phx::Memory::NewArray<Type>(Memory::g_Frame, count)

#define phx_delete(allocator, ptr)                  phx::Memory::Delete(allocator, ptr)