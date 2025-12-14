#pragma once

#include <PhxCore/Handle.h>
#include <PhxResource/ResourceTypes.h>

namespace phx
{
    // -- Implementation in ResourceMamanger.cpp -- 
    void ResourceIncRef(GenericHandle h);
    void ResourceDecRef(GenericHandle h);
    
    template<typename T>
    class ResourcePtr
    {
    public:
        ResourcePtr() = default;
        ResourcePtr(Handle<T> handle)
            : m_handle(handle)
        {
            if (m_handle.IsValid())
            {
                ResourceIncRef(GenericHandle::From(m_handle));
            }
        }
        ResourcePtr(const ResourcePtr& other)
            : m_handle(other.m_handle)
        {
            if (m_handle.IsValid())
            {
                ResourceIncRef(GenericHandle::From(m_handle));
            }
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
                Reset(); // Release current
                m_handle = other.m_handle;
                if (m_handle.IsValid())
                {
                    ResourceIncRef(GenericHandle::From(m_handle));
                }
            }

            return *this;
        }

        ResourcePtr& operator=(ResourcePtr&& other) noexcept
        {
            if (this != &other)
            {
                Reset(); // Release current
                m_handle = other.m_handle;
                other.m_handle = Handle<T>();
            }

            return *this;
        }

        // -- Implementation in ResourceMamanger.h --
        T* operator->() const;
        T* Get() const;

        bool IsValid() const { return m_handle.IsValid(); }
        Handle<T> GetHandle() const { return m_handle; }
        operator Handle<T>() const { return m_handle; }
        explicit operator bool() const { return m_handle.IsValid(); }

    private:
        void Reset()
        {
            if (m_handle.IsValid())
            {
                ResourceDecRef(GenericHandle::From(m_handle));
                m_handle = Handle<T>();
            }
        }

    private:
        Handle<T> m_handle;
    };
}