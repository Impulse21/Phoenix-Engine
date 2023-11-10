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
	void Present(SwapChain const& swapchainsToPresent);
	void WaitForIdle();

	CommandList* BeginCommandList(RHI::CommandListType type = RHI::CommandListType::Graphics);
	uint64_t SubmitCommands(CommandList* commandList);

	class Factory
	{
	public:

		static bool CreateSwapChain(SwapchainDesc const& desc, void* windowHandle, SwapChain& out);
		static bool CreateTexture(TextureDesc const& desc, Texture& out);
		static bool CreateGpuBuffer(BufferDesc const& desc, Buffer& out);

		template<typename T>
		static bool CreateCommandSignature(CommandSignatureDesc const& desc, CommandSignature& out)
		{
			static_assert(sizeof(T) % sizeof(uint32_t) == 0);
			return Factory::CreateCommandSignature(desc, sizeof(T), out);
		}

		// Use static class so we can get internals of the passed in types via friend
		static bool CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride);
		static bool CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode);
		static bool CreateInputLayout(Core::Span<VertexAttributeDesc> descs);
		static bool CreateGfxPipeline(GfxPipelineDesc const& desc);
		static bool CreateComputePipeline(ComputePipelineDesc const& desc);
		static bool CreateMeshPipeline(MeshPipelineDesc const& desc);
	};
}