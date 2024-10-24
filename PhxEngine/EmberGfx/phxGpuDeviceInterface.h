#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"
#include "EmberGfx/phxCommandCtxInterface.h"

namespace phx::gfx
{
	class IGpuDevice
	{
	public:
		virtual ~IGpuDevice() = default;

		virtual void Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle = nullptr) = 0;
		virtual void Finalize() = 0;

		virtual ShaderFormat GetShaderFormat() const = 0;

		virtual void WaitForIdle() = 0;
		virtual void ResizeSwapChain(SwapChainDesc const& swapChainDesc) = 0;

		virtual ICommandCtx* BeginCommandCtx(phx::gfx::CommandQueueType type = CommandQueueType::Graphics) = 0;
		virtual void SubmitFrame() = 0;

		virtual DynamicMemoryPage AllocateDynamicMemoryPage(size_t pageSize) = 0;
		virtual DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV, int subResource = -1) = 0;

	public:
		virtual ShaderHandle CreateShader(ShaderDesc const& desc) = 0;
		virtual void DeleteShader(ShaderHandle handle) = 0;

		virtual PipelineStateHandle CreatePipeline(PipelineStateDesc2 const& desc, RenderPassInfo* renderPassInfo = nullptr) = 0;
		virtual void DeletePipeline(PipelineStateHandle handle) = 0;

		virtual BufferHandle CreateBuffer(BufferDesc const& desc) = 0;
		virtual void DeleteBuffer(BufferHandle handle) = 0;

		virtual TextureHandle CreateTexture(TextureDesc const& desc, SubresourceData* initialData = nullptr) = 0;
		virtual void DeleteTexture(TextureHandle handle) = 0;
	};
}