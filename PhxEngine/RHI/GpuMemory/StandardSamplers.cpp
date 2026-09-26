#include "StandardSamplers.h"

#include <PhxEngine/RHI/RHI.h>

using namespace phx;
using namespace phx::rhi;

namespace
{
    SamplerDescriptor MakeSampler(SamplerFilter filter, SamplerAddressMode address, bool aniso, bool compare) noexcept
    {
        SamplerDescriptor desc = {
            .min_filter = filter,
            .mag_filter = filter,
            .mip_filter = filter,
            .address_u  = address,
            .address_v  = address,
            .address_w  = address,
        };

        if (aniso)
        {
            desc.anisotropy_enable = true;
            desc.max_anisotropy    = 16.0f;
        }

        if (compare)
        {
            desc.compare_enable = true;
            desc.compare_func   = ComparisonFunc::Less;
            desc.address_u      = SamplerAddressMode::Border;
            desc.address_v      = SamplerAddressMode::Border;
            desc.address_w      = SamplerAddressMode::Border;
            desc.border_colour  = SamplerBorderColour::OpaqueWhite;
        }

        return desc;
    }
}

void phx::rhi::WriteStandardSamplers(GpuCpuRange<byte> heap, u64 descriptor_size) noexcept
{
    PHX_ASSERT(heap.size >= kStandardSamplerCount * descriptor_size);

    const SamplerDescriptor samplers[kStandardSamplerCount] = {
        MakeSampler(SamplerFilter::Linear, SamplerAddressMode::Clamp, false, false), // LinearClamp
        MakeSampler(SamplerFilter::Linear, SamplerAddressMode::Wrap,  false, false), // LinearWrap
        MakeSampler(SamplerFilter::Point,  SamplerAddressMode::Clamp, false, false), // PointClamp
        MakeSampler(SamplerFilter::Point,  SamplerAddressMode::Wrap,  false, false), // PointWrap
        MakeSampler(SamplerFilter::Linear, SamplerAddressMode::Clamp, true,  false), // AnisoClamp
        MakeSampler(SamplerFilter::Linear, SamplerAddressMode::Wrap,  true,  false), // AnisoWrap
        MakeSampler(SamplerFilter::Linear, SamplerAddressMode::Border, false, true), // ShadowPCF
    };

    for (u32 i = 0; i < kStandardSamplerCount; ++i)
    {
        void* dest = OffsetPointer(heap.cpu, static_cast<u64>(i) * descriptor_size);
        WriteSamplerDescriptor(samplers[i], dest);
    }
}
