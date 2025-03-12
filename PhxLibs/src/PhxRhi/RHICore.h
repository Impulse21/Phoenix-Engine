#pragma once

#include "RHITypes.h"
#include "RHICommandCtx.h"

namespace phx::rhi
{
    struct RhiCreateInfo
    {
        SwapChainDescriptor SwapChianDesc = {};
        void* WindowsHandle = nullptr;
		uint32_t MaxNumTextures = 1000;
		uint32_t MaxNumGpuBuffers = 1000;
		uint32_t MaxNumPipelineStates = 1000;
    };

    void Initialize(RhiCreateInfo const& createInfo);
    void Finalize();

	Budget GetBudget();
    void WaitForIdle();

	ShaderFormat GetShaderFormat();

	CommandCtx* BeginCommnadCtx(CommandQueueType queueType = CommandQueueType::Graphics);

	void Present();

	// TODO Context management;

	TextureHandle CreateTexture(TextureDescriptor const& desc, MemInfo* initData = nullptr);
	PipelineStateHandle CreatePipelineState(PipelineStateDescriptor const& desc);
	GpuBufferHandle CreateBuffer(GpuBufferDescriptor const& desc, MemInfo* initData = nullptr);

	void DeletePipeline(PipelineStateHandle handle);
	void DeleteTexture(TextureHandle handle);
	void DeleteBuffer(GpuBufferHandle handle);

	DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV);
}