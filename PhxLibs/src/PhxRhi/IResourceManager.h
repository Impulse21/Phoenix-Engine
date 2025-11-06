#pragma once

#include <PhxRhi/RHICommon.h>

namespace phx::rhi
{
	class IResourceManager
	{
	public:
		virtual ~IResourceManager() = default;

		// -- Gpu Buffers  ---
		virtual BufferHandle CreateBuffer(const BufferDescriptor& desc, const void* initialData = nullptr) = 0;
		virtual void DeleteBuffer(BufferHandle handle) = 0;

		// -- Textures ---
		virtual TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr) = 0;
		virtual void DeleteTexture(TextureHandle handle) = 0;

		// -- Pipeline States ---
		virtual PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc) = 0;
		virtual void DeletePipeline(PipelineStateHandle handle) = 0;
	};
}