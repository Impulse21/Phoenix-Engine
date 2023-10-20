#pragma once

#include <RHI/RHIResources.h>
#include <Core/Memory.h>
#include <Core/Span.h>

namespace PhxEngine::RHI
{
	struct RHIParams
	{
		size_t ResourcePoolSize = PhxKB(1);
	};
	bool Initialize(RHIParams const& params);
	bool Finalize();
	void RunGarbageCollection();

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
	bool CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride, CommandSignature& out);
	bool CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode, Shader& out);
	bool CreateInputLayout(InputLayoutDesc const& desc, uint32_t attributeCount, InputLayout& out);
	bool CreateGfxPipeline(GfxPipelineDesc const& desc, Texture& out);
	bool CreateComputePipeline(ComputePipelineDesc const& desc, Texture& out);
	bool CreateMeshPipeline(MeshPipelineDesc const& desc, Texture& out);
	bool CreateGpuBuffer(GpuBufferDesc const& desc, Texture& out, void* initalData = nullptr);
	bool CreateTexture(TextureDesc const& desc, Texture& out, void* initalData = nullptr);
}