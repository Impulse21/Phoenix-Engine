#pragma once
#pragma once

#include <assert.h>
#include <atomic>

namespace PhxEngine::RHI
{
    //////////////////////////////////////////////////////////////////////////
    // RefPtr
    // Mostly a copy of Microsoft::WRL::ComPtr<T>
    //////////////////////////////////////////////////////////////////////////

    template <typename T>
    class RefPtr
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
        template<class U> friend class RefPtr;

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

        RefPtr() noexcept : ptr_(nullptr)
        {
        }

        RefPtr(std::nullptr_t) noexcept : ptr_(nullptr)
        {
        }

        template<class U>
        RefPtr(U* other) noexcept : ptr_(other)
        {
            InternalAddRef();
        }

        RefPtr(const RefPtr& other) noexcept : ptr_(other.ptr_)
        {
            InternalAddRef();
        }

        // copy ctor that allows to instanatiate class when U* is convertible to T*
        template<class U>
        RefPtr(const RefPtr<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept :
            ptr_(other.ptr_)

        {
            InternalAddRef();
        }

        RefPtr(RefPtr&& other) noexcept : ptr_(nullptr)
        {
            if (this != reinterpret_cast<RefPtr*>(&reinterpret_cast<unsigned char&>(other)))
            {
                Swap(other);
            }
        }

        // Move ctor that allows instantiation of a class when U* is convertible to T*
        template<class U>
        RefPtr(RefPtr<U>&& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept :
            ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        ~RefPtr() noexcept
        {
            InternalRelease();
        }

        RefPtr& operator=(std::nullptr_t) noexcept
        {
            InternalRelease();
            return *this;
        }

        RefPtr& operator=(T* other) noexcept
        {
            if (ptr_ != other)
            {
                RefPtr(other).Swap(*this);
            }
            return *this;
        }

        template <typename U>
        RefPtr& operator=(U* other) noexcept
        {
            RefPtr(other).Swap(*this);
            return *this;
        }

        RefPtr& operator=(const RefPtr& other) noexcept  // NOLINT(bugprone-unhandled-self-assignment)
        {
            if (ptr_ != other.ptr_)
            {
                RefPtr(other).Swap(*this);
            }
            return *this;
        }

        template<class U>
        RefPtr& operator=(const RefPtr<U>& other) noexcept
        {
            RefPtr(other).Swap(*this);
            return *this;
        }

        RefPtr& operator=(RefPtr&& other) noexcept
        {
            RefPtr(static_cast<RefPtr&&>(other)).Swap(*this);
            return *this;
        }

        template<class U>
        RefPtr& operator=(RefPtr<U>&& other) noexcept
        {
            RefPtr(static_cast<RefPtr<U>&&>(other)).Swap(*this);
            return *this;
        }

        void Swap(RefPtr&& r) noexcept
        {
            T* tmp = ptr_;
            ptr_ = r.ptr_;
            r.ptr_ = tmp;
        }

        void Swap(RefPtr& r) noexcept
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
        static RefPtr<T> Create(T* other)
        {
            RefPtr<T> Ptr;
            Ptr.Attach(other);
            return Ptr;
        }

        unsigned long Reset()
        {
            return InternalRelease();
        }
    };    // RefPtr

    //////////////////////////////////////////////////////////////////////////
    // RefCounter<T>
    // A class that implements reference counting in a way compatible with RefPtr.
    // Intended usage is to use it as a base class for interface implementations, like so:
    // class Texture : public RefCounter { ... }
    //////////////////////////////////////////////////////////////////////////

    template<class T>
    class RefCounter : public T
    {
    private:
        std::atomic<unsigned long> m_refCount = 1;
    public:
        virtual unsigned long AddRef()
        {
            return ++m_refCount;
        }

        virtual unsigned long Release()
        {
            unsigned long result = --m_refCount;
            if (result == 0) 
            {
                delete this;
            }
            return result;
        }
    };
}