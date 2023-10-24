#pragma once

#include <RHI/RHIResources.h>
#include <RHI/RHIEnums.h>
#include <Core/Memory.h>
#include <Core/Span.h>

namespace PhxEngine::RHI
{
	struct RHIParams
	{
		size_t NumInflightFrames = 3;
		size_t ResourcePoolSize = PhxKB(1);
	};

	bool Initialize(RHIParams const& params);
	bool Finalize();

	// Singles the end of the frame.
	// Presents provided swapchains,
	void EndFrame(Core::Span<SwapChain> swapchainsToPresent);

	GfxContext& BeginGfxCtx();
	ComputeContext& BeginComputeCtx();
	CopyContext& BeginCopyCtx();

	SubmitRecipt Submit(Core::Span<CommandContext> context);

	// Resource Creation Functions
	template<typename T>
	bool CreateCommandSignature(CommandSignatureDesc const& desc, CommandSignature& out)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0);
		return this->CreateCommandSignature(desc, sizeof(T), out);
	}
	
	// Use static class so we can get internals of the passed in types via friend
	CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride);
	SwapChainHandle CreateSwapChain(SwapchainDesc desc, void* windowHandle);
	ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode);
	InputLayoutHandle CreateInputLayout(VertexAttributeDesc const& desc, uint32_t attributeCount);
	GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc);
	ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc);
	MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc);
	BufferHandle CreateGpuBuffer(BufferDesc const& desc, void* initalData = nullptr);
	TextureHandle CreateTexture(TextureDesc const& desc, void* initalData = nullptr);

	void ResizeSwapChain(SwapChainHandle handle);
}