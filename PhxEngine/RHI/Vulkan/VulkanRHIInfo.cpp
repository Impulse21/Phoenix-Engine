#include "RHIVulkan.h"

namespace phx::rhi
{
    [[nodiscard]] ShaderFormat GetShaderFormat()
    {
        return ShaderFormat::Spirv;
    }

    [[nodiscard]] bool IsClipSpaceYDown()
    {
        return true;
    }
}