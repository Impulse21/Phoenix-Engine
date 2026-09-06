#pragma once

namespace phx
{

    template <typename T, typename TOffset = uint32_t>
    struct RelativePtr
    {
        TOffset Offset;

        void Set(void* ptr)
        {
            Offset = static_cast<size_t>(ptrdiff_t(ptr) - ptrdiff_t(this));
        }

        T* Get()
        {
            return (T*)(((char*)this) + Offset);
        }
        const T* Get() const
        {
            return (const T*)(((char*)this) + Offset);
        }

        operator T*()
        {
            return Get();
        }
        T* operator->()
        {
            return Get();
        }
        T const* operator->() const
        {
            return Get();
        }
    };
}  // namespace phx