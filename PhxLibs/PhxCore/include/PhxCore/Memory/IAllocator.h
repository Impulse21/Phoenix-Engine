#pragma once

#include <cstdint>
#include <PhxCore/Span.h>

namespace phx
{

	struct NonCopyable
	{
		NonCopyable& operator=(const NonCopyable&) = delete;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable() = default;
	};

	class IAllocator : NonCopyable
	{
	public:
		virtual ~IAllocator() = default;

		[[nodiscard]] virtual void* Allocate(size_t size, size_t alignment) = 0;
		[[nodiscard]] virtual void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) = 0;
		virtual bool IsAddressInRange(const void* ptr) const = 0;
		virtual void Deallocate(void* pointer) = 0;

        template <typename T, typename... Args>
        T* NewObject(Args&&... args) 
        {
            T* ptr = reinterpret_cast<T*>(Allocate(sizeof(T), alignof(T)));
            try 
            {
                // Use placement new to construct the object in the allocated memory
                ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
            }
            catch (...) 
            {
                // If the constructor throws, deallocate the memory to prevent leaks
                Deallocate(ptr);
                throw;
            }
            return ptr;
        }

        template <typename T>
		void DeleteObject(T* ptr)
		{
			ptr->~T(); // Call the destructor
			Deallocate(ptr);
		}
	};

	template<typename T>
	SpanMutable<T> AllocateArray(IAllocator* allocator, size_t size)
	{
		T* ptr = reinterpret_cast<T*>(allocator->Allocate(sizeof(T) * size, alignof(T)));
		return SpanMutable(ptr, size);
	}
}