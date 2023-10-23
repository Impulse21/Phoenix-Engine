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
	class Factory
	{
	public:
		static bool CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride, CommandSignature& out);
		static bool CreateSwapChain(SwapchainDesc desc, void* windowHandle, SwapChain& out);
		static bool CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode, Shader& out);
		static bool CreateInputLayout(InputLayoutDesc const& desc, uint32_t attributeCount, InputLayout& out);
		static bool CreateGfxPipeline(GfxPipelineDesc const& desc, Texture& out);
		static bool CreateComputePipeline(ComputePipelineDesc const& desc, Texture& out);
		static bool CreateMeshPipeline(MeshPipelineDesc const& desc, Texture& out);
		static bool CreateGpuBuffer(GpuBufferDesc const& desc, Texture& out, void* initalData = nullptr);
		static bool CreateTexture(TextureDesc const& desc, Texture& out, void* initalData = nullptr);
	};
}