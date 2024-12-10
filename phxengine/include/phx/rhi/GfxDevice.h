#pragma once

#include <deque>
#include <functional>

#include "phx/core/Memory.h"
#include "phx/rhi/RHITypes.h"
#include "phx/rhi/ResourcePool.h"

#include "PlatformTypes.h"

namespace phx::rhi
{
    class GfxCommandListRecorder;


    using CommandListPool = ResourcePool<CommandList, platform::CommandListResource>;
    using SwapChainPool= ResourcePool<SwapChain, platform::SwapChainResource, platform::SwapChainBindings>;
    using TexturePool = ResourcePool<Texture, platform::TextureResource, platform::TextureBindings>;
    using GpuBufferPool = ResourcePool<GpuBuffer, platform::GpuBufferResource, platform::GpuBufferBindings>;
    using PipelineStatePool = ResourcePool<PipelineState, platform::PipelineStateResource>;

    class GfxDevice
    {
    public:
        inline static GfxDevice* Ptr = nullptr;
        static void Initialize();
        static void Finalize();

    public:
        GfxDevice(rhi::GfxDeviceDescriptor const& descriptor);
        ~GfxDevice();

        ShaderFormat GetShaderFormat() const
        {
            return m_platformDevice.GetShaderFormat();
        }

        void Present(phx::Span<SwapChainHandle> swapChains)
        {
            assert(swapChains.Size() == 1 && "Only support a single swap chain");
            m_platformDevice.Present(
                *m_swapChainPool.Get<platform::SwapChainResource>(swapChains[0]),
                *m_swapChainPool.Get<platform::SwapChainBindings>(swapChains[0]));
        }

    public:
        void Submit(Span<CommandListHandle> handles)
        {
            ScopedScratchMarker executeCommnadList;

            Memory::ScratchAllocator& allocator =  Memory::GetScratchAllocator();
            auto* resources = allocator.AllocArray<platform::CommandListResource*>(handles.Size());

            for (size_t i = 0; i < handles.Size(); i++)
            {
                resources[i] = m_commandListPool.Get<platform::CommandListResource>(handles[i]);
            }

            // Build a list of data
            m_platformDevice.Submit(Span<platform::CommandListResource*>(resources, handles.size()));
        }


    public:
        CommandListHandle CreateGfxCommandList() { return CreateCommandList(CommandQueueType::Graphics); }
        CommandListHandle CreateComputeCommadList() { return CreateCommandList(CommandQueueType::Compute); }
        CommandListHandle CreateCopyCommandList() { return CreateCommandList(CommandQueueType::Copy); }

        CommandListHandle CreateCommandList(CommandQueueType type)
        {
            CommandListHandle handle = m_commandListPool.Allocate();

            m_platformDevice.CreateCommandList(
                type,
                *m_commandListPool.Get<platform::CommandListResource>(handle));

            return handle;
        }

        void DeleteCommandList(CommandListHandle handle)
        {
            m_commandListPool.Free(handle);
        }

        SwapChainHandle CreateSwapChain(SwapChainDescriptor const& desc)
        {
            SwapChainHandle handle = m_swapChainPool.Allocate();

            m_platformDevice.CreateSwapChain(
                desc,
                *m_swapChainPool.Get<platform::SwapChainResource>(handle),
                *m_swapChainPool.Get<platform::SwapChainBindings>(handle));

            return handle;
        }

        void CreateSwapChain(SwapChainDescriptor const& desc, SwapChainHandle handle)
        {
            m_platformDevice.CreateSwapChain(
                desc,
                *m_swapChainPool.Get<platform::SwapChainResource>(handle),
                *m_swapChainPool.Get<platform::SwapChainBindings>(handle));
        }

        void DeleteSwapChain(SwapChainHandle swapChain)
        {
            DeferredItem d =
            {
                m_frameCount,
                [=]()
                {
                    m_swapChainPool.Free(swapChain);
                }
            };

            m_deferredQueue.push_back(d);
        }

        PipelineStateHandle CreatePipeline(PipelineStateDescriptor const& desc)
        {
            PipelineStateHandle handle = m_pipelineStatePool.Allocate();

            m_platformDevice.CreatePipeline(
                desc,
                *m_pipelineStatePool.Get<platform::PipelineStateResource>(handle));

            return handle;
        }

        void DeletePipeline(PipelineStateHandle handle)
        {
            DeferredItem d =
            {
                m_frameCount,
                [=]()
                {
                    m_pipelineStatePool.Free(handle);
                }
            };

            m_deferredQueue.push_back(d);
        }

        TextureHandle CreateTexture(TextureDescriptor const& desc, MemInfo* initData = nullptr)
        {
            TextureHandle handle = m_texturePool.Allocate();

            m_platformDevice.CreateTexture(
                desc,
                *m_texturePool.Get<platform::TextureResource>(handle),
                *m_texturePool.Get<platform::TextureBindings>(handle),
                initData);

            return handle;
        }

        void DeleteTexture(TextureHandle handle)
        {
            DeferredItem d =
            {
                m_frameCount,
                [=]()
                {
                    m_texturePool.Free(handle);
                }
            };

            m_deferredQueue.push_back(d);
        }

        DescriptorIndex GetDescriptorIndex(TextureHandle texture, SubresouceType type = SubresouceType::SRV)
        {
            UNREFERENCED_PARAMETER(texture);
            UNREFERENCED_PARAMETER(type);

            return cInvalidDescriptorIndex;
        }

    public:
        CommandListPool& GetCommandListPool() { return m_commandListPool; }
        SwapChainPool& GetSwapChainPool() { return m_swapChainPool; }
        TexturePool& GetTexturPool() { return  m_texturePool; }
        GpuBufferPool& GetGpuBufferPool() { return  m_gpuBufferPool; }
        PipelineStatePool& GetPipelineStatePool() { return  m_pipelineStatePool; }


    private:
        platform::GfxDevice m_platformDevice;

        CommandListPool m_commandListPool;
        SwapChainPool m_swapChainPool;
        TexturePool m_texturePool;
        GpuBufferPool m_gpuBufferPool;
        PipelineStatePool m_pipelineStatePool;

        uint64_t m_frameCount = 0;
        struct DeferredItem
        {
            uint64_t Frame;
            std::function<void()> DeferredFunc;
        };
        std::deque<DeferredItem> m_deferredQueue;
    };
    
    // TODO: Create Device.
}