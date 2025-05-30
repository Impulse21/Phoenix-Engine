#pragma once

#include <cstdlib>

#include <PhxRhi/RHICommon.h> 

namespace phx::rhi
{
    class CommandBuffer;
}

namespace phx::rhi
{

    struct GfxDeviceDescriptor
    {
        SwapChainDescriptor SwapChainDesc = {};
        void* WindowsHandle = nullptr;
        uint32_t MaxNumTextures = 1000;
        uint32_t MaxNumGpuBuffers = 1000;
        uint32_t MaxNumPipelineStates = 1000;
    };

    template<typename TDerivedDevice>
    class BaseGfxDevice
    {
    public:
        bool Initialize(GfxDeviceDescriptor const& desc)
        {
            return static_cast<TDerivedDevice*>(this)->PlatformInitialize(desc);
        }

        void Shutdown() // Or Deinitialize / Finalize if you prefer
        {
            static_cast<TDerivedDevice*>(this)->PlatformShutdown();
        }

        CommandBuffer* BeginCommandBuffer()
        {
            return static_cast<TDerivedDevice*>(this)->PlatformBeginCommandBuffer();
        }

        void Present()
        {
            static_cast<TDerivedDevice*>(this)->PlatformPresent();
        }

        void WaitForIdle()
        {
            static_cast<TDerivedDevice*>(this)->PlatformWaitForIdle();
        }

        GpuBufferHandle CreateBuffer(const GpuBufferDescriptor& desc, const void* initialData = nullptr)
        {
            return static_cast<TDerivedDevice*>(this)->PlatformCreateBuffer(desc, initialData);
        }

        TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr)
        {
            return static_cast<TDerivedDevice*>(this)->PlatformCreateTexture(desc, initialData);
        }

        PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc)
        {
            return static_cast<TDerivedDevice*>(this)->PlatformCreatePipeline(desc);
        }

        void DeletePipeline(PipelineStateHandle handle)
        {
            static_cast<TDerivedDevice*>(this)->PlatformDeletePipeline(handle);
        }

        void DeleteTexture(TextureHandle handle)
        {
            static_cast<TDerivedDevice*>(this)->PlatformDeleteTexture(handle);
        }

        void DeleteBuffer(GpuBufferHandle handle)
        {
            static_cast<TDerivedDevice*>(this)->PlatformDeleteBuffer(handle);
        }

        DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV) const
        {
            return static_cast<const TDerivedDevice*>(this)->PlatformGetDescriptorIndex(handle, type);
        }

        Budget GetBudget() const
        {
            return static_cast<const TDerivedDevice*>(this)->PlatformGetBudget();
        }

        ShaderFormat GetShaderFormat() const
        {
            return static_cast<const TDerivedDevice*>(this)->PlatformGetShaderFormat();
        }

    protected:
        BaseGfxDevice() = default;
        ~BaseGfxDevice() = default;
    };
}