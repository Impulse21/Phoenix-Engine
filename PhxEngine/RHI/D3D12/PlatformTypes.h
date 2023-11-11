#pragma once

#include <RHI/D3D12/D3D12CommandList.h>
#include <RHI/D3D12/D3D12Resources.h>
#include <RHI/D3D12/D3D12SwapChain.h>

namespace PhxEngine::RHI
{
	using PlatformInputLayout					= D3D12::D3D12InputLayout;
	using PlatformCommandList					= D3D12::D3D12CommandList;
	using PlatformTexture						= D3D12::D3D12Texture;
	using PlatformBuffer						= D3D12::D3D12Buffer;
	using PlatformSwapChain						= D3D12::D3D12SwapChain;
	using PlatformCommandSignature				= D3D12::D3D12CommandSignature;
	using PlatformShader						= D3D12::D3D12Shader;
	using PlatformGfxPipeline					= D3D12::D3D12GfxPipeline;
	using PlatformComputePipeline				= D3D12::D3D12ComputePipeline;
	using PlatformMeshPipeline					= D3D12::D3D12MeshPipeline;
	using PlatformRTAccelerationStructure		= D3D12::D3D12RTAccelerationStructure;
	using PlatformTimerQuery					= D3D12::D3D12TimerQuery;

	struct PlatformBufferShaderView				: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };
	struct PlatformBufferTypedShaderView		: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };
	struct PlatformBufferUnorderedView			: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };
	struct PlatformBufferTypedUnorderedView		: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };

	struct PlatformTextureRenderTargetView		: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };
	struct PlatformTextureDepthStencilView		: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };
	struct PlatformTextureShaderView			: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; }; 
	struct PlatformTextureUnorderedAccessView	: public D3D12::D3D12Descriptor { using D3D12::D3D12Descriptor::D3D12Descriptor; };
}