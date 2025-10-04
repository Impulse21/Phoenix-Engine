#pragma once

#include <atomic>
#include <cassert>
#include <PhxData/TemplatedTypeId.h>

namespace phx::data
{
#if true
    template<typename T>
    class DataPtr
    {
    public:

    private:
        void* m_data;
    };

    template<typename T>
    class DataRef
    {
    public:
    private:
        void* m_data;
    };

#else // TODO: Remove
    struct AnyPtr
    {
        // Data
        void* ValuePtr = nullptr;
        TemplateTypeId TypeId = kEmptyTypeId;

        // Operators
        auto operator<=>(const AnyPtr& other) const noexcept = default;
    };

    struct ControlBlockBase
    {
        std::atomic<size_t> RefCount = 1;
        virtual ~ControlBlockBase() = default;
    };

    template<typename U>
    struct ControlBlock final : public ControlBlockBase
    {
        U* Ptr;

        ControlBlock(U* p) : Ptr(p) {}
        ~ControlBlock()
        {
            delete Ptr;
        }
    };

    template<typename T>
    class RefPtr
    {
        template<typename>
        friend class RefPtr;

    public:
        template<typename... Args>
        static inline RefPtr<T> Create(Args&&... args)
        {
            return RefPtr<T>(new T(std::forward<Args>(args)...));
        }

    public:
        RefPtr()
            : control(nullptr), Ptr(nullptr)
        {
        }

        RefPtr(T* raw)
            : control(raw ? new ControlBlock<T>(raw) : nullptr), Ptr(raw)
        {
        }

        RefPtr(const RefPtr& other)
            : control(other.control), Ptr(other.Ptr)
        {
            if (control)
                control->RefCount.fetch_add(1, std::memory_order_relaxed);
        }

        template<typename U> requires std::convertible_to<U*, T*>
        RefPtr(const RefPtr<U>& other)
            : control(other.control), Ptr(other.Ptr)
        {
            if (control)
                control->RefCount.fetch_add(1, std::memory_order_relaxed);
        }

        RefPtr(RefPtr&& other) noexcept
            : control(other.control), Ptr(other.Ptr)
        {
            other.control = nullptr;
            other.Ptr = nullptr;
        }

        RefPtr& operator=(const RefPtr& other)
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                Ptr = other.Ptr;
                if (control)
                    control->RefCount.fetch_add(1, std::memory_order_relaxed);
            }
            return *this;
        }

        RefPtr& operator=(RefPtr&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                Ptr = other.Ptr;
                other.control = nullptr;
                other.Ptr = nullptr;
            }
            return *this;
        }

        ~RefPtr()
        {
            Release();
        }

        T* Get() const
        {
            return Ptr;
        }

        T& operator*()
        {
            assert(Ptr);
            return *Ptr;
        }
        const T& operator*() const
        {
            assert(Ptr);
            return *Ptr;
        }

        T* operator->()
        {
            assert(Ptr);
            return Ptr;
        }

        const T* operator->() const
        {
            assert(Ptr);
            return Ptr;
        }

        explicit operator bool() const
        {
            return Ptr != nullptr;
        }

        size_t UseCount() const
        {
            return control ? control->RefCount.load(std::memory_order_relaxed) : 0;
        }

    private:
        void Release()
        {
            if (control)
            {
                if (control->RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    delete control;
                }
                control = nullptr;
                Ptr = nullptr;
            }
        }

    private:
        ControlBlockBase* control;
        T* Ptr;
    };
#endif
}