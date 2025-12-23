#pragma once

namespace phx
{
	template<typename T>
	class RefPtr
	{

    public:
        RefPtr() = default;

        explicit RefPtr(T* ptr)
            : m_ptr(ptr)
        {
            AddRef();
        }

        RefPtr(const RefPtr& other)
            : m_ptr(other.m_ptr)
        {
            AddRef();
        }

        RefPtr(RefPtr&& other) noexcept
            : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        ~RefPtr()
        {
            Release();
        }

        RefPtr& operator=(const RefPtr& other)
        {
            if (this != &other)
            {
                Release();
                m_ptr = other.m_ptr;
                AddRef();
            }
            return *this;
        }

        RefPtr& operator=(RefPtr&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        T* Get() const { return m_ptr; }
        T* operator->() const { return m_ptr; }
        explicit operator bool() const { return m_ptr != nullptr; }

    private:
        void AddRef()
        {
            if (m_ptr)
                m_ptr->AddRef();
        }

        void Release()
        {
            if (m_ptr)
                m_ptr->Release();
            m_ptr = nullptr;
        }

    private:
        T* m_ptr = nullptr;
    };
}