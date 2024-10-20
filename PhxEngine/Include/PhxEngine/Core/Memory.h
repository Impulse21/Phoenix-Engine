#pragma once

#include <string>
#include <atomic>
#include <limits>

// Credit to Raptor Engine: Using it for learning.

namespace PhxEngine { struct NewPlaceholder {}; };
inline void* operator new (size_t, PhxEngine::NewPlaceholder, void* where) { return where; }
inline void operator delete(void*, PhxEngine::NewPlaceholder, void*) {};

#define PhxKB(size)                 (size * 1024)
#define PhxMB(size)                 (size * 1024 * 1024)
#define PhxGB(size)                 (size * 1024 * 1024 * 1024)

#define PhxToKB(x)	((size_t) (x) >> 10)
#define PhxToMB(x)	((size_t) (x) >> 20)
#define PhxToGB(x)	((size_t) (x) >> 30)

void* operator new(size_t p_size, const char* p_description);
void* operator new(size_t p_size, void* (*p_allocfunc)(size_t p_size)); 

void* operator new(size_t p_size, void* p_pointer, size_t check, const char* p_description); 

#ifdef _MSC_VER
// When compiling with VC++ 2017, the above declarations of placement new generate many irrelevant warnings (C4291).
// The purpose of the following definitions is to muffle these warnings, not to provide a usable implementation of placement delete.
void operator delete(void* p_mem, const char* p_description);
void operator delete(void* p_mem, void* (*p_allocfunc)(size_t p_size));
void operator delete(void* p_mem, void* p_pointer, size_t check, const char* p_description);
#endif

#define phx_new(m_class) new ("") m_class
#define phx_delete(m_v) PhxEngine::Core::MemDelete(m_v)

#define phx_new_allocator(m_class, m_allocator) new (m_allocator::Allocate) m_class
#define phx_new_placement(m_placement, m_class) new (m_placement) m_class

#define phx_delete_notnull(m_v) \
	{                           \
		if (m_v)                \
        {                       \
			MemDelete(m_v);  \
		}                       \
	}                           \

#define phx_new_arr(m_class, m_count) PhxEngine::Core::MemNew_Arr<m_class>(m_count)
#define phx_realloc_arr(m_class, data, m_count) PhxEngine::Core::MemRealloc_Arr<m_class>(data, m_count)
#define phx_arr_len(m_v) PhxEngine::Core::NemArr_Len(m_v)
#define phx_delete_arr(m_v) PhxEngine::Core::MemDelete_Arr(m_v)


namespace PhxEngine::Core
{
    namespace SystemMemory
    {
        void* Alloc(size_t bytes, bool padAlign = false);
        void* AllocArray(size_t size, size_t count, bool padAlign = false);
        void* ReallocArray(void* memory, size_t size, size_t newCount, bool padAlign = false);
        void* Realloc(void* memory, size_t p_bytes, bool padAlign = false);
        void Free(void* ptr, bool padAlign = false);
        void Cleanup();

        uint64_t GetMemoryAvailable();
        uint64_t GetMemUsage();
        uint64_t GetMemMaxUsage();
        uint64_t GetNumActiveAllocations();
    }

	struct MemoryStatistics 
	{
		size_t AllocatedBytes;
		size_t TotalBytes;
		uint32_t AllocationCount;

		void add(size_t a) 
		{
			if (a)
			{
				this->AllocatedBytes += a;
				++this->AllocationCount;
			}
		}
	};

    template <typename T>
    inline constexpr T RoundDiv(T x, T y)
    {
        return (x + y / (T)2) / y;
    }

    template <typename T>
    inline constexpr T AlignUp(T val, T align)
    {
        return (val + align - 1) / align * align;
    }

    constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
    {
        return ((value + alignment - 1) / alignment) * alignment;
    }

	class IAllocator
	{
	public:
		virtual ~IAllocator() = default;

        template<class _T>
        _T* AllocateArray(size_t numElements, size_t alignment)
        {
            return static_cast<_T*>(this->AllocArray(sizeof(_T), numElements, alignment));
        };

        template<class _T>
        _T* Allocate(size_t alignment = 0)
        {
            return static_cast<_T*>(this->Allocate(sizeof(_T), alignment));
        };

        template<class _T>
        _T* RellocateArray(_T* memory, size_t numElements, size_t alignment)
        {
            return static_cast<_T*>(this->ReallocArray(memory, sizeof(_T), numElements, alignment));
        };

        template<class _T>
        _T* Reallocate(_T* memory, size_t alignment = 0)
        {
            return static_cast<_T*>(this->Allocate(memory, sizeof(_T), alignment));
        };

        virtual void Initialize(size_t size) = 0;
        virtual void Finalize() = 0;

		virtual void* Allocate(size_t size, size_t alignment) = 0;
        virtual void* AllocArray(size_t size, size_t count, size_t alignment) = 0;
		virtual void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) = 0;

        virtual void* Rellocate(void* ptr, size_t size, size_t alignment) = 0;
        virtual void* ReallocArray(void* ptr, size_t size, size_t count, size_t alignment) = 0;

		virtual void Free(void* pointer) = 0;
	};

    class DefaultAllocator : public IAllocator
    {
    public:
        void Initialize(size_t size) override {};
        void Finalize() override {};

        void* Allocate(size_t size, size_t alignment) override { return SystemMemory::Alloc(size); };
        void* AllocArray(size_t size, size_t count, size_t alignment) override { return SystemMemory::AllocArray(size, count); };
        void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override { return SystemMemory::Alloc(size); }

        void* Rellocate(void* ptr, size_t size, size_t alignment) override { return SystemMemory::Realloc(ptr, size); };
        void* ReallocArray(void* ptr, size_t size, size_t count, size_t alignment) override { return SystemMemory::ReallocArray(ptr, size, count); };

        void Free(void* ptr) override { SystemMemory::Free(ptr); }
    };

	class HeapAllocator : public IAllocator
	{
	public:
		void Initialize(size_t size) override;
		void Finalize() override;

		void BuildIU();


		void* Allocate(size_t size, size_t alignment) override;
        void* AllocArray(size_t size, size_t count, size_t alignment) override {};
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

        void* Rellocate(void* ptr, size_t size, size_t alignment);
        void* ReallocArray(void* ptr, size_t size, size_t count, size_t alignment);

		void Free(void* pointer) override;

	private:
		void* m_tlsfHandle;
		void* m_memory;
		size_t m_allocatedSize = 0;
		size_t m_maxSize = 0;
	};

	class StackAllocator : public IAllocator
	{
	public:
		void Initialize(size_t size);
		void Finalize();

		void* Allocate(size_t size, size_t alignment) override;
        void* AllocArray(size_t size, size_t count, size_t alignment) override {};
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

        void* Rellocate(void* ptr, size_t size, size_t alignment);
        void* ReallocArray(void* ptr, size_t size, size_t count, size_t alignment);

		void Free(void* pointer) override;

		size_t GetMarker();
		void FreeMarker(size_t marker);

		void Clear();

	private:
		uint8_t* m_memory = nullptr;
		size_t m_totalSize = 0;
		size_t m_allocatedSize = 0;
	};

	class LinearAllocator: public IAllocator
	{
	public:
		void Initialize(size_t size) override;
		void Finalize() override;

		void* Allocate(size_t size, size_t alignment) override;
        void* AllocArray(size_t size, size_t count, size_t alignment) override {};
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

        void* Rellocate(void* ptr, size_t size, size_t alignment);
        void* ReallocArray(void* ptr, size_t size, size_t count, size_t alignment);

		void Free(void* pointer) override;

		void Clear();

	private:
		uint8_t* m_memory = nullptr;
		size_t m_totalSize = 0;
		size_t m_allocatedSize = 0;
	};
	struct MemoryServiceConfiguration
	{
		size_t MaximumDynamicSize = PhxMB(32);
	};


    template <class T>
    void MemDelete(T* p_class)
    {
        if (!std::is_trivially_destructible<T>::value)
        {
            p_class->~T();
        }

        SystemMemory::Free(p_class);
    }

    template <class T, class A>
    void MemDelete_Allocator(T* p_class)
    {
        if (!std::is_trivially_destructible<T>::value)
        {
            p_class->~T();
        }

        A::Free(p_class);
    }

    template <typename T>
    T* MemNew_Arr(size_t p_elements, size_t padding = 1) 
    {
        if (p_elements == 0) 
        {
            return nullptr;
        }

        /** overloading operator new[] cannot be done , because it may not return the real allocated address (it may pad the 'element count' before the actual array). Because of that, it must be done by hand. This is the
        same strategy used by std::vector, and the Vector class, so it should be safe.*/

        uint64_t* mem = (uint64_t*)SystemMemory::AllocArray(sizeof(T), p_elements);
        T* failptr = nullptr; //get rid of a warning
        *(mem - 1) = p_elements;

        if (!std::is_trivially_constructible<T>::value) 
        {
            T* elems = (T*)mem;

            /* call operator new */
            for (size_t i = 0; i < p_elements; i++) 
            {
                new (&elems[i]) T;
            }
        }

        return (T*)mem;
    }

    /**
     * Wonders of having own array functions, you can actually check the length of
     * an allocated-with memnew_arr() array
     */

    template <typename T>
    size_t NemArr_Len(const T* p_class) 
    {
        uint64_t* ptr = (uint64_t*)p_class;
        return *(ptr - 1);
    }

    template <typename T>
    void MemDelete_Arr(T* p_class) 
    {
        uint64_t* ptr = (uint64_t*)p_class;

        if (!std::is_trivially_destructible<T>::value)
        {
            uint64_t elem_count = *(ptr - 1);

            for (uint64_t i = 0; i < elem_count; i++)
            {
                p_class[i].~T();
            }
        }

        SystemMemory::Free(ptr);
    }

    template <class T, class A>
    void MemDelete_Arr_Allocator(T* p_class)
    {
        uint64_t* ptr = (uint64_t*)p_class;

        if (!std::is_trivially_destructible<T>::value)
        {
            uint64_t elem_count = *(ptr - 1);

            for (uint64_t i = 0; i < elem_count; i++)
            {
                p_class[i].~T();
            }
        }

        A::Free(p_class);
    }

    template <typename T>
    T* MemRealloc_Arr(T* p_class, size_t newSize)
    {
        if (newSize == 0)
        {
            MemDelete_Arr(p_class);
            return nullptr;
        }

        /** overloading operator new[] cannot be done , because it may not return the real allocated address (it may pad the 'element count' before the actual array). Because of that, it must be done by hand. This is the
        same strategy used by std::vector, and the Vector class, so it should be safe.*/
        size_t prevLength = NemArr_Len<T>(p_class);
        uint64_t* mem = (uint64_t*)SystemMemory::ReallocArray(p_class, sizeof(T), newSize);
        T* failptr = nullptr; //get rid of a warning
        *(mem - 1) = newSize;

        if (!std::is_trivially_constructible<T>::value)
        {
            T* elems = (T*)mem;

            /* call operator new */
            for (size_t i = prevLength; i < newSize; i++)
            {
                new (&elems[i]) T;
            }
        }

        return (T*)mem;
    }

    template <typename T>
    class PhxStlAllocator 
    {
    public:
        // Type Definitions
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T         value_type;

        // Constructor (if needed)
        PhxStlAllocator() {}
        PhxStlAllocator(const PhxStlAllocator&) {}

        // Member Functions
        pointer allocate(size_type n)
        {
            // Allocate memory for n objects of type T
            return phx_new_arr(T, n);
        }

        void deallocate(pointer p, size_type n) noexcept
        {
            if (p)
            {
                phx_delete_arr(p);
            }
        }

        void construct(pointer p, const_reference val)
        {
            new (p) T(val);
        }

        void destroy(pointer p)
        {
            // p->~T();
        }

        size_type max_size() const noexcept
        {
            return size_t(-1);
        }

        pointer           address(reference x) const { return &x; }
        const_pointer     address(const_reference x) const { return &x; }
        PhxStlAllocator<T>& operator=(const PhxStlAllocator&) { return *this; }

        // Rebind Member Type
        template <class U>
        struct rebind { typedef PhxStlAllocator<U> other; };

        template <class U>
        PhxStlAllocator(const PhxStlAllocator<U>&) {}

        template <class U>
        PhxStlAllocator& operator=(const PhxStlAllocator<U>&) { return *this; }
    };

}

