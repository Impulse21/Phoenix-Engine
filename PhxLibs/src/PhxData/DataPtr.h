#pragma once

#include <atomic>
#include <cassert>

namespace phx::data
{
    template<typename T>
    class RefPtr
    {
    public:
        template<typename... Args>
        static inline RefPtr<T> Create(Args&&... args)
        {
            return RefPtr<T>(new T(std::forward<Args>(args)...));
        }

    public:
        RefPtr()
            : control(nullptr)
        {
        }

        RefPtr(T* ptr)
            : control(ptr ? new ControlBlock(ptr) : nullptr)
        {
        }

        RefPtr(const RefPtr& other)
            : control(other.control)
        {
            if (control)
            {
                control->refCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        RefPtr(RefPtr&& other) noexcept
            : control(other.control)
        {
            other.control = nullptr;
        }

        ~RefPtr()
        {
            Release();
        }

        template<typename U> requires std::convertible_to<U*, T*>
        RefPtr(const RefPtr<U>& other)
            : control(other.control->ptr ? new ControlBlock(other.control->ptr) : nullptr)
        {
            if (control)
                control->refCount.fetch_add(1, std::memory_order_relaxed);
        }

        RefPtr& operator=(const RefPtr& other)
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                if (control)
                {
                    control->refCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
            return *this;
        }

        RefPtr& operator=(RefPtr&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                other.control = nullptr;
            }
            return *this;
        }

        T* Get() const
        {
            return control ? control->ptr : nullptr;
        }

        T& operator*() const
        {
            assert(Get());
            return *Get();
        }

        T* operator->() const
        {
            assert(Get());
            return Get();
        }

        explicit operator bool() const
        {
            return Get() != nullptr;
        }

        size_t UseCount() const
        {
            return control ? control->refCount.load(std::memory_order_relaxed) : 0;
        }

    private:
        void Release()
        {
            if (control)
            {
                if (control->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    delete control;
                }
                control = nullptr;
            }
        }
    private:
        struct ControlBlock
        {
            std::atomic<size_t> refCount;
            T* ptr;

            ControlBlock(T* p)
                : refCount(1), ptr(p)
            {
            }

            ~ControlBlock()
            {
                delete ptr;
            }
        };

        ControlBlock* control;
    };
}