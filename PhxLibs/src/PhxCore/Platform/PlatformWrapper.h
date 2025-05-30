#pragma once

#include "PlatformConfig.h"

namespace phx::Platform
{
    inline platform::PlatformWrapper& Get()
    {
        static platform::PlatformWrapper s_instance;
        return s_instance;
    }
}