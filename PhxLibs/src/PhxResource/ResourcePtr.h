#pragma once
#include <PhxCore/Handle.h>
#include <PhxResource/ResourceSystem.h>

namespace phx
{
    template<typename T>
    class ResourcePtr
    {
    public:
        ResourcePtr() = default;
        ResourcePtr(Handle<T> handle) 
            : m_handle(handle)
        {
            if (m_handle.IsValid()) 
                ResourceSystem::IncRef(m_handle);
        }

        ResourcePtr(const ResourcePtr& other) 
            : m_handle(other.m_handle)
        {
            if (m_handle.IsValid()) 
                ResourceSystem::IncRef(m_handle);
        }

        ResourcePtr(ResourcePtr&& other) noexcept 
            : m_handle(other.m_handle)
        {
            other.m_handle = Handle<T>();
        }

        ~ResourcePtr()
        {
            Reset();
        }

        ResourcePtr& operator=(const ResourcePtr& other)
        {
            if (this != &other) 
            {
                Reset();
                m_handle = other.m_handle;
                if (m_handle.IsValid()) 
                    ResourceSystem::IncRef(m_handle);
            }

            return *this;
        }

        ResourcePtr& operator=(ResourcePtr&& other) noexcept
        {
            if (this != &other) 
            {
                Reset();
                m_handle = other.m_handle;
                other.m_handle = Handle<T>();
            }

            return *this;
        }

        T* operator->() const
        {
            T* ptr = ResourceSystem::Get(m_handle);
            return ptr ? ptr : ResourceSystem::GetDefault<T>();
        }

        T* Get() const { return ResourceSystem::Get(m_handle); }

        bool IsValid() const { return m_handle.IsValid(); }
        Handle<T> GetHandle() const { return m_handle; }

    private:
        void Reset()
        {
            if (m_handle.IsValid()) 
            {
                ResourceSystem::DecRef(m_handle);
                m_handle = Handle<T>();
            }
        }

    private:
        Handle<T> m_handle;
    };
}