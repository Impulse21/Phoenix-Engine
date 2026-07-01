#pragma once


namespace phx
{
    template<typename T>
    class FramePtr
    {
    public:
        PHX_NO_COPY_NO_MOVE(FramePtr);
        FramePtr() = default;
        explicit FramePtr(T* ptr) : m_ptr(ptr) {}

        [[nodiscard]] T* Get() { return m_ptr; }
        [[nodiscard]] T& operator*() { return m_ptr; }
        [[nodiscard]] T* operator->() { return m_ptr; }

        [[nodiscard]] bool IsValid() { return m_ptr != nullptr; }
        explicit operator bool() { return IsValid(); }
        
    private:
        T* m_ptr = nullptr;
    }
}