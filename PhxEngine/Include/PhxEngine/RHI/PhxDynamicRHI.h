#pragma once

#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/RHI/PhxRHIResources.h>

namespace PhxEngine::RHI
{
    class ScopedMarker;

    struct ExecutionReceipt
    {
        uint64_t FenceValue;
        CommandQueueType CommandQueue;
    };

    struct MemoryUsage
    {
        uint64_t Budget = 0ull;
        uint64_t Usage = 0ull;
    };


    class DynamicRHI
    {
    public:
        virtual ~DynamicRHI() = default;

        // -- Frame Functions ---
    public:
        virtual void Initialize() = 0;
        virtual void Finalize() = 0;

        // -- Submits Command lists and presents ---
        virtual void Present(SwapChain* swapChain) = 0;
        virtual void WaitForIdle() = 0;

        virtual bool IsDevicedRemoved() = 0;

        virtual bool CheckCapability(DeviceCapability deviceCapability) = 0;

        // -- Resouce Functions ---
    public:
        virtual SwapChainRef CreateSwapChain(SwapChainDesc const& desc, void* windowsHandle) = 0;
        virtual void ResizeSwapChain(SwapChain* swapChain, SwapChainDesc const& desc) = 0;
    };

    template <class T>
    void HashCombine(size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

}