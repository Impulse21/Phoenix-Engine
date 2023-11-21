#pragma once

#include <vector>
#include <PhxEngine/RHI/PhxRHIDefinitions.h>

#include <PhxEngine/Core/RefCountPtr.h>

namespace PhxEngine::RHI
{
    struct SwapChainDesc
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t BufferCount = 3;
        RHI::Format Format = RHI::Format::R10G10B10A2_UNORM;
        bool Fullscreen = false;
        bool VSync = false;
        bool EnableHDR = false;
        RHI::ClearValue OptmizedClearValue =
        {
            .Colour =
            {
                0.0f,
                0.0f,
                0.0f,
                1.0f,
            }
        };
    };

    class RHIResource
    {
    protected:
        RHIResource() = default;
        virtual ~RHIResource() = default;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        RHIResource(const RHIResource&) = delete;
        RHIResource(const RHIResource&&) = delete;
        RHIResource& operator=(const RHIResource&) = delete;
        RHIResource& operator=(const RHIResource&&) = delete;
    };

    class SwapChain : public RHIResource
    {
    public:
        SwapChain() = default;
        const SwapChainDesc& Desc() const { return this->m_desc; }

    protected:
        SwapChainDesc m_desc;
    };
    using SwapChainRef = Core::RefCountPtr<SwapChain>;

    class CommandList : public RHIResource
    {
    public:
        CommandList() = default;

    };
    using CommandListRef = Core::RefCountPtr<CommandList>;
}