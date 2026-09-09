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
            return reinterpret_cast<T*>(reinterpret_cast<uptr>(this) + Offset);
        }
        const T* Get() const
        {
            return reinterpret_cast<const T*>(reinterpret_cast<uptr>(this) + Offset);
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