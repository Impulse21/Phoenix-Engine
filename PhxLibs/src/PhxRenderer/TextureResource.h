#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/PhxRhi.h>
#include <PhxResource/FileFormatUtils.h>

namespace phx::renderer
{
    struct MipLevelInfo
    {
        uint64_t offset_in_uncompressed;
    };

    // todo: adjust to match the rhi::TextureDescriptor
    struct TextureMetadata
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_layers;
        uint32_t mip_levels;

        rhi::Format format;
        FileFormat::RelativePtr<MipLevelInfo> mip_info;
    };

	struct TextureResource : public Resource
	{
        rhi::TextureHandle texture_handle;

		PHX_DECLARE_RESOURCE(TextureResource);

		~TextureResource() override;

		bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index) override;
	};
}