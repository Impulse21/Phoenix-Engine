#pragma once

#include "Dx12GfxDevice.h"

namespace phx::rhi::platform
{
	using GfxDevice 			= dx12::GfxDeviceDx12;

	using SwapChain_Hot 		= dx12::SwapChain_Hot;
	using SwapChain_Cold 		= dx12::SwapChain_Cold;

	using PipelineState_Hot 	= dx12::PipelineState_Hot;
	using PipelineState_Cold 	= dx12::PipelineState_Cold;

	using Texture_Hot 			= dx12::Texture_Hot;
	using Texture_Cold 			= dx12::Texture_Cold;

	struct TextureShaderView 		: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
	struct TextureUnorderedView 	: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
	struct TextureRenderTargetView 	: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
	struct TextureDepthStencilView	: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };

	using GpuBuffer_Hot 		= dx12::GpuBuffer_Hot;
	using GpuBuffer_Cold 		= dx12::GpuBuffer_Cold;

	struct GpuBufferShaderView 		: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
	struct GpuBufferTypedShaderView 	: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
	struct GpuBufferUnorderedView 		: public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
	struct GpuBufferTypedUnorderedView : public dx12::TypedCPUDescriptorHandle { using dx12::TypedCPUDescriptorHandle::TypedCPUDescriptorHandle; };
}