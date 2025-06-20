#pragma once

#include <cstdlib>
#include <deque>

#include <PhxRhi/RHICommon.h> 

namespace phx
{
    class IAllocator;
}
namespace phx::rhi
{
    class GfxCommandCtx;
    class ComputeCommandCtx;
    class CopyCommandCtx;
}

namespace phx::rhi
{
    struct DeferredItem
    {
        uint64_t frame;
        std::function<void()> deferred_func;
    };

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

        CommandBuffer* BeginCommandBuffer(phx::IAllocator* frame_arena)
        {
            return static_cast<TDerivedDevice*>(this)->PlatformBeginCommandBuffer(frame_arena);
        }

        void Present()
        {
            static_cast<TDerivedDevice*>(this)->PlatformPresent();
        }

        void WaitForIdle()
        {
            static_cast<TDerivedDevice*>(this)->PlatformWaitForIdle();
        }

        CopyCommandCtx* BeginCopyContext()
        {
            return static_cast<TDerivedDevice>(this)->PlatformBeginCopyCtx();
        }

        void SubmitCopyAndWait(CopyCommandCtx* ctx)
        {
            return static_cast<TDerivedDevice>(this)->PlatformSubmitCopyAndWait();
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
            EnqueueDelete({
                m_frame_count,
                [this, handle]()
                {
                    static_cast<TDerivedDevice*>(this)->PlatformDeletePipeline(handle);
                }
            });
        }

        void DeleteTexture(TextureHandle handle)
        {
            EnqueueDelete({
                m_frame_count,
                [this, handle]()
                {
                    static_cast<TDerivedDevice*>(this)->PlatformDeleteTexture(handle);
                }
            });
        }

        void DeleteBuffer(GpuBufferHandle handle)
        {
            EnqueueDelete({
                m_frame_count,
                [this, handle]()
                {
                    static_cast<TDerivedDevice*>(this)->PlatformDeleteBuffer(handle);
                }
            });
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

        void EnqueueDelete(DeferredItem&& item)
        {
            m_deferredQueue.emplace_back(std::forward<DeferredItem>(item));
        }

        void ProcessDeletionQueue(uint64_t completed_frame)
        {
            while (!m_deferredQueue.empty())
            {
                DeferredItem& DeferredItem = m_deferredQueue.front();
                if (DeferredItem.frame + kBufferCount < completed_frame)
                {
                    DeferredItem.deferred_func();
                    m_deferredQueue.pop_front();
                }
                else
                {
                    break;
                }
            }
        }

    protected:
        uint64_t m_frame_count;
        std::deque<DeferredItem> m_deferredQueue;
    };
}