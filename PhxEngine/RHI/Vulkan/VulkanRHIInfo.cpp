#include "RHIVulkan.h"

namespace phx::rhi
{
    [[nodiscard]] constexpr ShaderFormat GetShaderFormat()
    {
        return ShaderFormat::Spirv;
    }
}