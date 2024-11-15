#pragma once

#include <deque>
#include <functional>

#include "RHITypes.h"
#include "RHIPlatformTypes.h"
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
        GfxDevice();
        ~GfxDevice();

        ShaderFormat GetShaderFormat() const
        {
            return m_platformDevice.GetShaderFormat();
        }

    public:
        PipelineStateHandle CreatePipeline(PipelineStateDescriptor const& desc)
        {
            platform::PipelineState_Hot hot;
            platform::PipelineState_Cold cold;

            m_platformDevice.CreatePipeline(desc, hot, cold);
            return m_pipelineStatePool.Allocate(hot, cold);
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

    private:
        platform::GfxDevice m_platformDevice;

        ResourcePool<Texture, platform::Texture_Hot, platform::Texture_Cold> m_texturePool;
        ResourcePool<GpuBuffer, platform::GpuBuffer_Hot, platform::GpuBuffer_Cold> m_gpuBufferPool;
        ResourcePool<PipelineState, platform::PipelineState_Hot, platform::PipelineState_Cold> m_pipelineStatePool;

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