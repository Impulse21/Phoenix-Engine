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
	void Present(Core::Span<SwapChainHandle> swapchainsToPresent);
	void WaitForIdle();

	CommandList* BeginCommandList(RHI::CommandListType type = RHI::CommandListType::Graphics);
	uint64_t SubmitCommands(CommandList* commandList);

	// Resource Creation Functions
	template<typename T>
	bool CreateCommandSignature(CommandSignatureDesc const& desc, CommandSignature& out)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0);
		return this->CreateCommandSignature(desc, sizeof(T), out);
	}
	
	// Use static class so we can get internals of the passed in types via friend
	CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride);
	SwapChainHandle CreateSwapChain(SwapchainDesc const& desc);
	ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode);
	InputLayoutHandle CreateInputLayout(Core::Span<VertexAttributeDesc> descs);
	GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc);
	ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc);
	MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc);
	BufferHandle CreateGpuBuffer(BufferDesc const& desc, void* initalData = nullptr);
	TextureHandle CreateTexture(TextureDesc const& desc, void* initalData = nullptr);
	RenderPassHandle CreateRenderPass(RenderPassDesc desc);

	void DeleteCommandSignature(CommandSignatureHandle  handle);
	void DeleteSwapChain(SwapChainHandle handle);
	void DeleteShader(ShaderHandle handle);
	void DeleteInputLayout(InputLayoutHandle handle);
	void DeleteGfxPipeline(GfxPipelineHandle handle);
	void DeleteComputePipeline(ComputePipelineHandle handle);
	void DeleteMeshPipeline(MeshPipelineHandle handle);
	void DeleteGpuBuffer(BufferHandle handle);
	void DeleteTexture(TextureHandle handle);
	void DeleteRTAccelerationStructure(RTAccelerationStructureHandle  handle);
	void DeleteTimerQuery(TimerQueryHandle handle);

	void ResizeSwapChain(SwapChainHandle handle, SwapchainDesc const& desc);
	RHI::Format GetSwapChainFormat(SwapChainHandle handle);

	const TextureDesc& GetTextureDesc(TextureHandle handle);
	const BufferDesc& GetBufferDesc(BufferHandle handle);

	DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1);
	DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1);
}