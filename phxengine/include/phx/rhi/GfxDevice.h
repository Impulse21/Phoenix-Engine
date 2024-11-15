#pragma once

#include <deque>
#include <functional>

#include "RHITypes.h"
#include "RHIPlatformTypes.h"

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
            return {};
        }

        void DeletePipeline(PipelineStateHandle handle)
        {

        }

    public:
        CommandCtx& BeginCommandCtx(CommandQueueType type = CommandQueueType::Graphics)
        {
            return m_commandCtx[0];
        }
        
    private:
        platform::GfxDevice m_platformDevice;
        std::vector<CommandCtx> m_commandCtx;

        struct DeferredItem
        {
            uint64_t Frame;
            std::function<void()> DeferredFunc;
        };
        std::deque<DeferredItem> m_deferredQueue;
    };
    
    // TODO: Create Device.
}