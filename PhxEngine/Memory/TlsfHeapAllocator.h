#pragma once

#include <PhxEngine/Memory/IHeapAllocator.h>

namespace phx
{
    class TlsfHeapAllocator : public IHeapAllocator
    {
    public:
        TlsfHeapAllocator() = default;
        ~TlsfHeapAllocator() override { Shutdown(); }

        PHX_NO_COPY_NO_MOVE(TlsfHeapAllocator);

    public:
        void Initialize(void* block, usize block_size);
        void Shutdown();

        [[nodiscard]] void* Alloc(usize size, usize alignment = 8) override;
        void Free(void* ptr) override;

        [[nodiscard]] usize GetBlockSize() const { return m_block_size; }

    private:
        void* m_tlsf = nullptr;
        usize m_block_size = 0;
    };
}