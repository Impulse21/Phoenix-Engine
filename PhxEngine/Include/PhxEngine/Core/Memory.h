#pragma once

#include <string>
#include <atomic>

// Credit to Raptor Engine: Using it for learning.

#define phx_new(allocator, ...) new (PhxEngine::NewPlaceholder(), allocator.Allocate(sizeof(__VA_ARGS__), alignof(__VA_ARGS__))) __VA_ARGS__
#define phx_delete(allocator, var) allocator.Deallocate(var);

namespace PhxEngine { struct NewPlaceholder {}; };
inline void* operator new (size_t, PhxEngine::NewPlaceholder, void* where) { return where; }
inline void operator delete(void*, PhxEngine::NewPlaceholder, void*) {};

#define PhxKB(size)                 (size * 1024)
#define PhxMB(size)                 (size * 1024 * 1024)
#define PhxGB(size)                 (size * 1024 * 1024 * 1024)

#define PhxToKB(x)	((size_t) (x) >> 10)
#define PhxToMB(x)	((size_t) (x) >> 20)
#define PhxToGB(x)	((size_t) (x) >> 30)

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

	class IAllocator
	{
	public:
		virtual ~IAllocator() = default;

		virtual void* Allocate(size_t size, size_t alignment) = 0;
		virtual void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) = 0;

		virtual void Deallocate(void* pointer) = 0;
	};

	class HeapAllocator : public IAllocator
	{
	public:
		void Initialize(size_t size);
		void Finalize();

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
		void Initialize(size_t size);
		void Finalize();

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

	namespace MemoryService
	{
		void Initialize(PhxEngine::Core::MemoryServiceConfiguration const& config);
		void Finalize();

		IAllocator& GetScratchAllocator();
		IAllocator& GetSystemAllocator();
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
}

