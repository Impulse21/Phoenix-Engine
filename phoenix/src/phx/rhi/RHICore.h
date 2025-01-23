#pragma once

#include "RHITypes.h"

namespace phx::rhi
{
    struct RhiCreateInfo
    {
        SwapChainDescriptor SwapChianDesc = {};
        void* WindowsHandle = nullptr;
		uint32_t MaxNumTextures = 1000;
		uint32_t MaxNumPipelineStates = 1000;
    };

    void Initialize(RhiCreateInfo const& createInfo);
    void Finalize();

    void WaitForIdle();

	ShaderFormat GetShaderFormat();

	void Present();

	// TODO Context management;

	TextureHandle CreateTexture(TextureDescriptor const& desc, MemInfo* initData = nullptr);
	PipelineStateHandle CreatePipelineState(PipelineStateDescriptor const& desc);
	GpuBufferHandle CreateBuffer(GpuBufferDescriptor const& desc, MemInfo* initData = nullptr);

	void DeletePipeline(PipelineStateHandle handle);
	void DeleteTexture(TextureHandle handle);
	void DeleteBuffer(GpuBufferHandle handle);

	DescriptorIndex GetDescriptorIndex(TextureHandle texture, SubresouceType type = SubresouceType::SRV);
}