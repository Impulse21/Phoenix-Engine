#pragma once

#include <PhxEngine/RHI/RHITypes.h>
#include "Shaders/StandardSamplerInterop.h"

namespace phx::renderer
{
    constexpr u32 kStandardSamplerCount = static_cast<u32>(StandardSampler::Count);

    void WriteStandardSamplers(rhi::GpuCpuRange<byte> heap, u64 descriptor_size) noexcept;
}
