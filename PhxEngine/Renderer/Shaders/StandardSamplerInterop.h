#pragma once

#include "PhxInterop.h"

// -- namespace ---
#ifdef __cplusplus
#include <PhxEngine/RHI/RHITypes.h>

namespace phx::renderer
{
#endif


#ifdef  __cplusplus
    enum class StandardSampler : u32
    {
            LinearClamp,
            LinearWrap,
            PointClamp,
            PointWrap,
            AnisoClamp,
            AnisoWrap,
            ShadowPCF,
            Count
    };

    inline constexpr rhi::DescriptorIndex ToDescriptorIndex(StandardSampler v)
    {
        return static_cast<rhi::DescriptorIndex>(v);
    }

#else

    enum StandardSampler : uint
    {
            LinearClamp,
            LinearWrap,
            PointClamp,
            PointWrap,
            AnisoClamp,
            AnisoWrap,
            ShadowPCF,
            Count
    };
#endif

#ifdef __cplusplus
}
#endif
