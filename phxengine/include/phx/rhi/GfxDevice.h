#pragma once

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
    };
    
    // TODO: Create Device.
}