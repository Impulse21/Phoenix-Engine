#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::rhi
{
    enum class StandardSampler : u32
    {
        LinearClamp,
        LinearWrap,
        PointClamp,
        PointWrap,
        AnisoClamp,
        AnisoWrap,
        ShadowPCF,
        Count,
    };

    constexpr u32 kStandardSamplerCount = static_cast<u32>(StandardSampler::Count);

    void WriteStandardSamplers(GpuCpuRange<byte> heap, u64 descriptor_size) noexcept;
}
