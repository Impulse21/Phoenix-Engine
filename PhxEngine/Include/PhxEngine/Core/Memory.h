#pragma once

#include <string>
#include <atomic>

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

void* operator new(size_t p_size, const char* p_description); ///< operator new that takes a description and uses MemoryStaticPool
void* operator new(size_t p_size, void* (*p_allocfunc)(size_t p_size)); ///< operator new that takes a description and uses MemoryStaticPool

void* operator new(size_t p_size, void* p_pointer, size_t check, const char* p_description); ///< operator new that takes a description and uses a pointer to the preallocated memory

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

#define phx_new_arr(m_class, m_count) PhxEngine::Core::MemNew_Arr_Template<m_class>(m_count)
#define phx_arr_len(m_v) PhxEngine::Core::NemArr_Len(m_v)
#define phx_delete_arr(m_v) PhxEngine::Core::MemDelete_Arr(m_v)

namespace PhxEngine::Core
{

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
            return static_cast<_T*>(this->Allocate(numElements * sizeof(_T), alignment));
        };

        template<class _T>
        _T* Allocate(size_t alignment = 0)
        {
            return static_cast<_T*>(this->Allocate(sizeof(_T), alignment));
        };


        virtual void Initialize(size_t size) = 0;
        virtual void Finalize() = 0;

		virtual void* Allocate(size_t size, size_t alignment) = 0;
		virtual void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) = 0;

		virtual void Deallocate(void* pointer) = 0;
	};

	class HeapAllocator : public IAllocator
	{
	public:
		void Initialize(size_t size) override;
		void Finalize() override;

		void BuildIU();


		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

		void Deallocate(void* pointer) override;

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
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

		void Deallocate(void* pointer) override;

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
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

		void Deallocate(void* pointer) override;

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

	namespace SystemMemory
	{
		void Initialize(PhxEngine::Core::MemoryServiceConfiguration const& config);
		void Finalize();

		IAllocator& GetAllocator();
	}


    //////////////////////////////////////////////////////////////////////////
    // RefCountPtr
    // Mostly a copy of Microsoft::WRL::ComPtr<T>
    //////////////////////////////////////////////////////////////////////////

    template <typename T>
    class RefCountPtr
    {
    public:
        typedef T InterfaceType;

        template <bool b, typename U = void>
        struct EnableIf
        {
        };

        template <typename U>
        struct EnableIf<true, U>
        {
            typedef U type;
        };

    protected:
        InterfaceType* ptr_;
        template<class U> friend class RefCountPtr;

        void InternalAddRef() const noexcept
        {
            if (ptr_ != nullptr)
            {
                ptr_->AddRef();
            }
        }

        unsigned long InternalRelease() noexcept
        {
            unsigned long ref = 0;
            T* temp = ptr_;

            if (temp != nullptr)
            {
                ptr_ = nullptr;
                ref = temp->Release();
            }

            return ref;
        }

    public:

        RefCountPtr() noexcept : ptr_(nullptr)
        {
        }

        RefCountPtr(std::nullptr_t) noexcept : ptr_(nullptr)
        {
        }

        template<class U>
        RefCountPtr(U* other) noexcept : ptr_(other)
        {
            InternalAddRef();
        }

        RefCountPtr(const RefCountPtr& other) noexcept : ptr_(other.ptr_)
        {
            InternalAddRef();
        }

        // copy ctor that allows to instanatiate class when U* is convertible to T*
        template<class U>
        RefCountPtr(const RefCountPtr<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept :
            ptr_(other.ptr_)

        {
            InternalAddRef();
        }

        RefCountPtr(RefCountPtr&& other) noexcept : ptr_(nullptr)
        {
            if (this != reinterpret_cast<RefCountPtr*>(&reinterpret_cast<unsigned char&>(other)))
            {
                Swap(other);
            }
        }

        // Move ctor that allows instantiation of a class when U* is convertible to T*
        template<class U>
        RefCountPtr(RefCountPtr<U>&& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept :
            ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        ~RefCountPtr() noexcept
        {
            InternalRelease();
        }

        RefCountPtr& operator=(std::nullptr_t) noexcept
        {
            InternalRelease();
            return *this;
        }

        RefCountPtr& operator=(T* other) noexcept
        {
            if (ptr_ != other)
            {
                RefCountPtr(other).Swap(*this);
            }
            return *this;
        }

        template <typename U>
        RefCountPtr& operator=(U* other) noexcept
        {
            RefCountPtr(other).Swap(*this);
            return *this;
        }

        RefCountPtr& operator=(const RefCountPtr& other) noexcept  // NOLINT(bugprone-unhandled-self-assignment)
        {
            if (ptr_ != other.ptr_)
            {
                RefCountPtr(other).Swap(*this);
            }
            return *this;
        }

        template<class U>
        RefCountPtr& operator=(const RefCountPtr<U>& other) noexcept
        {
            RefCountPtr(other).Swap(*this);
            return *this;
        }

        RefCountPtr& operator=(RefCountPtr&& other) noexcept
        {
            RefCountPtr(static_cast<RefCountPtr&&>(other)).Swap(*this);
            return *this;
        }

        template<class U>
        RefCountPtr& operator=(RefCountPtr<U>&& other) noexcept
        {
            RefCountPtr(static_cast<RefCountPtr<U>&&>(other)).Swap(*this);
            return *this;
        }

        void Swap(RefCountPtr&& r) noexcept
        {
            T* tmp = ptr_;
            ptr_ = r.ptr_;
            r.ptr_ = tmp;
        }

        void Swap(RefCountPtr& r) noexcept
        {
            T* tmp = ptr_;
            ptr_ = r.ptr_;
            r.ptr_ = tmp;
        }

        [[nodiscard]] T* Get() const noexcept
        {
            return ptr_;
        }

        operator T* () const
        {
            return ptr_;
        }

        InterfaceType* operator->() const noexcept
        {
            return ptr_;
        }

        T** operator&()   // NOLINT(google-runtime-operator)
        {
            return &ptr_;
        }

        [[nodiscard]] T* const* GetAddressOf() const noexcept
        {
            return &ptr_;
        }

        [[nodiscard]] T** GetAddressOf() noexcept
        {
            return &ptr_;
        }

        [[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
        {
            InternalRelease();
            return &ptr_;
        }

        T* Detach() noexcept
        {
            T* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }

        // Set the pointer while keeping the object's reference count unchanged
        void Attach(InterfaceType* other)
        {
            if (ptr_ != nullptr)
            {
                auto ref = ptr_->Release();
                (void)ref;

                // Attaching to the same object only works if duplicate references are being coalesced. Otherwise
                // re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
                assert(ref != 0 || ptr_ != other);
            }

            ptr_ = other;
        }

        // Create a wrapper around a raw object while keeping the object's reference count unchanged
        static RefCountPtr<T> Create(T* other)
        {
            RefCountPtr<T> GPtr;
            GPtr.Attach(other);
            return GPtr;
        }

        unsigned long Reset()
        {
            return InternalRelease();
        }
    };    // RefCountPtr

    //////////////////////////////////////////////////////////////////////////
    // RefCounter<T>
    // A class that implements reference counting in a way compatible with RefCountPtr.
    // Intended usage is to use it as a base class for interface implementations, like so:
    // class Texture : public RefCounter<ITexture> { ... }
    //////////////////////////////////////////////////////////////////////////

    template<class T>
    class RefCounter : public T
    {
    private:
        std::atomic<unsigned long> m_refCount = 1;

    public:
        virtual unsigned long AddRef() override
        {
            return ++m_refCount;
        }

        virtual unsigned long Release() override
        {
            unsigned long result = --m_refCount;
            if (result == 0) {
                delete this;
            }
            return result;
        }
    };


    template <class T>
    void MemDelete(T* p_class)
    {
        if (!std::is_trivially_destructible<T>::value)
        {
            p_class->~T();
        }

        SystemMemory::GetAllocator().Deallocate(p_class);
    }

    template <class T, class A>
    void MemDelete_Allocator(T* p_class)
    {
        if (!std::is_trivially_destructible<T>::value)
        {
            p_class->~T();
        }

        A::free(p_class);
    }

    template <typename T>
    T* MemNew_Arr_Template(size_t p_elements, size_t padding = 1) 
    {
        if (p_elements == 0) 
        {
            return nullptr;
        }

        /** overloading operator new[] cannot be done , because it may not return the real allocated address (it may pad the 'element count' before the actual array). Because of that, it must be done by hand. This is the
        same strategy used by std::vector, and the Vector class, so it should be safe.*/

        size_t len = sizeof(T) * p_elements;
        uint64_t* mem = (uint64_t*)SystemMemory::GetAllocator().Allocate(len, padding);
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

        SystemMemory::GetAllocator().Deallocate(ptr);
    }

}

