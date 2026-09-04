#pragma once

namespace phx
{
    template<usize BufferSize = 64>
    class FixedCallable
    {
    public:
        FixedCallable() = default;

        template<typename TFn>
        FixedCallable(TFn&& fn)
        {
            static_assert(sizeof(TFn) <= BufferSize,
                "FixedCallable: lambda captures exceed buffer size. "
                "Reduce captures or increase BufferSize.");

            static_assert(std::is_trivially_destructible_v<TFn>,
                "FixedCallable: callable must be trivially destructible. "
                "No owning captures (std::string, std::vector, shared_ptr etc.)");

            ::new(m_buffer) TFn(std::forward<TFn>(fn));
            m_invoke = [](const void* buf) 
            { 
                (*static_cast<const TFn*>(buf))(); 
            };
        }

        void operator()() const
        {
            PHX_ASSERT(m_invoke != nullptr);
            m_invoke(m_buffer);
        }

        [[nodiscard]] bool IsValid() const { return m_invoke != nullptr; }

    private:
        alignas(8) u8       m_buffer[BufferSize] = {};
        void (*m_invoke)(const void*)            = nullptr;
    };
}