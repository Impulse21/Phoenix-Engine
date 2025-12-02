#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/PhxRhi.h>

namespace phx::renderer
{
    struct MipLevelInfo
    {
        uint64_t offset_in_heap;
        uint64_t compressedSize;    // Size in the file
        uint64_t uncompressedSize;  // Size in VRAM
        uint32_t width;
        uint32_t height;
    };

    struct TextureMetadata
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_layers;
        uint32_t mip_levels;

        rhi::Format format;

        // Followed immediately by:
        // MipLevelInfo mipTable[mipLevels];
    };

	struct TextureResource : public Resource
	{
		PHX_DECLARE_RESOURCE(TextureResource);

		rhi::TextureHandle TextureHandle;

		~TextureResource() override;

		bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index) override;
	};
}