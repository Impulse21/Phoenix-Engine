#pragma once

#include <PhxEngine/Memory/IHeapAllocator.h>

namespace phx
{
    class TlsfAllocator : public IHeapAllocator
    {
    public:
        TlsfAllocator() = default;
        ~TlsfAllocator() { Shutdown(); };

        PHX_NO_COPY_NO_MOVE(TlsfAllocator);

    public:
        void Initialize(void* block, usize block_size);
        void Shutdown();

        [[nodiscard]] void* Alloc(usize size, usize alignment = 8) override;
        [[nodiscard]] void Free(void* ptr) override;

        [[nodiscard]] usize GetBlockSize() const { return m_block_size; }

    private:
        void* m_tlsf = nullptr;
        usize m_block_size = 0;
    };
}