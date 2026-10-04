#pragma once

#include "PhxInterop.h"

// -- namespace ---
#ifdef __cplusplus
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
