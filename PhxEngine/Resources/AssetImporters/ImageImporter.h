#pragma once

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Resources/Intermediate/IntermediateTexture.h>

namespace phx::resources
{
    IntermediateTexture ImportImage(Span<const u8> encoded_bytes);
}
