#pragma once

#include <PhxEngine/Memory/IHeapAllocator.h>

// This is disabled for now. At the time I was trying to be cute
// had control heap allocations. This is causing to much upfront costs.
// I will stick with system memory for core allocations.
#if false
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
#endif