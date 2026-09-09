#include "ImageImporter.h"

#include <PhxEngine/Core/Log.h>

#include <stb_image.h>

#include <cstring>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "ImageImporter" };
}

IntermediateTexture phx::resources::ImportImage(Span<const u8> encoded_bytes)
{
    IntermediateTexture texture;

    int width = 0, height = 0, channels_in_file = 0;
    stbi_uc* decoded = stbi_load_from_memory(
        encoded_bytes.data(), static_cast<int>(encoded_bytes.Size()),
        &width, &height, &channels_in_file, 4);

    if (!decoded)
    {
        PHX_LOG_ERROR(k_log, "Failed to decode image ({0} bytes): {1}", encoded_bytes.Size(), stbi_failure_reason());
        return texture;
    }

    const size_t byte_size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    texture.pixel_data = MemoryBuffer(byte_size);
    std::memcpy(texture.pixel_data.Data(), decoded, byte_size);
    stbi_image_free(decoded);

    texture.width  = static_cast<uint32_t>(width);
    texture.height = static_cast<uint32_t>(height);
    texture.format = rhi::Format::RGBA8_UNORM;

    return texture;
}
