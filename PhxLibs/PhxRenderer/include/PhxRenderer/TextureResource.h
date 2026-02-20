#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>
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

    struct TextureResource final : public Resource
	{
        rhi::TextureHandle texture_handle;

		void Dispose() override;
        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index) override;

        PHX_DECLARE_RESOURCE(TextureResource)
	};

    static_assert(sizeof(TextureResource) <= 64);

}

PHX_DEFINE_RESOURCE(
    phx::renderer::TextureResource,
    ".phxtex",                      // Extension
    "TextureLoader"                 // Loader ID
);