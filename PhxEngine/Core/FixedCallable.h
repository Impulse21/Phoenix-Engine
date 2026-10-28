#pragma once

namespace phx
{
    template<usize BufferSize = 64>
    class FixedCallable
    {
    public:
        FixedCallable() = default;

        template<typename TFn>
            requires(!std::is_same_v<std::decay_t<TFn>, FixedCallable>)
        FixedCallable(TFn&& fn)
        {
            using T = std::decay_t<TFn>;

            static_assert(sizeof(T) <= BufferSize,
                "FixedCallable: lambda captures exceed buffer size. "
                "Reduce captures or increase BufferSize.");

            static_assert(alignof(T) <= kAlign,
                "FixedCallable: callable alignment exceeds buffer alignment.");

            static_assert(std::is_trivially_copyable_v<T>,
                "FixedCallable: callable must be trivially copyable. "
                "No owning captures (std::string, std::vector, shared_ptr etc.)");

            ::new(m_buffer) T(std::forward<TFn>(fn));
            m_invoke = [](const void* buf) 
            { 
                (*static_cast<const T*>(buf))(); 
            };
        }

        void operator()() const
        {
            PHX_ASSERT(m_invoke != nullptr);
            m_invoke(m_buffer);
        }

        [[nodiscard]] bool IsValid() const { return m_invoke != nullptr; }

    private:
        static constexpr usize kAlign = 8;

        alignas(kAlign) u8       m_buffer[BufferSize] = {};
        void (*m_invoke)(const void*)            = nullptr;
    };
}