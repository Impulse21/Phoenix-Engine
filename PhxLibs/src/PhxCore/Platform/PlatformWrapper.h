#pragma once

#include "PlatformConfig.h"

namespace phx::platform
{
    inline PlatformWrapper& GetPlatform()
    {
        inline static PlatformWrapper s_instance;
        return s_instance;
    }
}