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
		virtual ICommandCtx* BeginCommandCtx(phx::gfx::CommandQueueType type = CommandQueueType::Graphics) = 0;
		virtual void SubmitFrame() = 0;

	public:
		virtual ShaderHandle CreateShader(ShaderDesc const& desc) = 0;
		virtual void DeleteShader(ShaderHandle handle) = 0;

		virtual PipelineStateHandle CreatePipeline(PipelineStateDesc2 const& desc) = 0;
		virtual void DeletePipeline(PipelineStateHandle handle) = 0;
	};
}