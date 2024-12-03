#pragma once

#include <deque>
#include <functional>

#include "RHITypes.h"
#include "PlatformTypes.h"
#include "phx/rhi/ResourcePool.h"

#include "CommandCtx.h"

namespace phx::rhi
{
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
            auto& platform = *m_swapChainPool.Get(swapChains[0]);
            m_platformDevice.Present(platform);
        }
    public:
        SwapChainHandle CreateSwapChain(SwapChainDescriptor const& desc)
        {
            SwapChainHandle handle = m_swapChainPool.Emplace();

            m_platformDevice.CreateSwapChain(desc, *m_swapChainPool.Get(handle));
            return handle;
        }

        void CreateSwapChain(SwapChainDescriptor const& desc, SwapChainHandle handle)
        {
            auto& swapchain = *m_swapChainPool.Get(handle);
            m_platformDevice.CreateSwapChain(desc, swapchain);
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
            PipelineStateHandle handle = m_pipelineStatePool.Emplace();
            auto& pipeline = *m_pipelineStatePool.Get(handle);
            m_platformDevice.CreatePipeline(desc, pipeline);

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
            TextureHandle handle = m_texturePool.Emplace();
            auto& texture = *m_texturePool.Get(handle);
            m_platformDevice.CreateTexture(desc, texture, initData);

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

    private:
        platform::GfxDevice m_platformDevice;

        ResourcePool<SwapChain, platform::SwapChain> m_swapChainPool;
        ResourcePool<Texture, platform::Texture> m_texturePool;
        ResourcePool<GpuBuffer, platform::GpuBuffer> m_gpuBufferPool;
        ResourcePool<PipelineState, platform::PipelineState> m_pipelineStatePool;

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