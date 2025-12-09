#pragma once

#include <PhxResource/ResourceFwds.h>
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

    struct TextureResource final : public ResourceHotData
	{
        rhi::TextureHandle texture_handle;
		~TextureResource();

	};
    static_assert(sizeof(TextureResource) <= 64);

    struct TextureColdData final : public ResourceColdData
    {

    };
}

namespace phx::texture_ops
{
    bool CollectPendingGpuTransitions(TextureResourceHandle texture_handle, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index);
}

PHX_DEFINE_RESOURCE(
    renderer::TextureResource,      // T
    renderer::TextureResource,      // Hot
    renderer::TextureColdData,      // Cold
    ".phxmsh",                      // Extension
    "MeshLoader"                    // Loader ID
);