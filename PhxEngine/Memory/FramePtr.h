#pragma once


namespace phx
{
    template<typename T>
    class FramePtr
    {
    public:
        PHX_NO_COPY_NO_MOVE(FramePtr);
        FramePtr() = default;
        FramePtr(std::nullptr_t) : m_ptr(nullptr) {}
        explicit FramePtr(T* ptr) : m_ptr(ptr) {}

        [[nodiscard]] T* Get() { return m_ptr; }
        [[nodiscard]] T& operator*() { return m_ptr; }
        [[nodiscard]] T* operator->() { return m_ptr; }
        [[nodiscard]] T& operator[](usize i)   const { return m_ptr[i]; }

        [[nodiscard]] bool IsValid() { return m_ptr != nullptr; }
        explicit operator bool() { return IsValid(); }
        
    private:
        T* m_ptr = nullptr;
    };
}