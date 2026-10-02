#pragma once

#include "PhxInterop.h"

// -- namespace ---
#ifdef __cplusplus
namespace phx::renderer
{
#endif

    PHX_ENUM_DEF(StandardSampler,
        LinearClamp,
        LinearWrap,
        PointClamp,
        PointWrap,
        AnisoClamp,
        AnisoWrap,
        ShadowPCF,
        Count
    )


#ifdef __cplusplus
}
#endif
