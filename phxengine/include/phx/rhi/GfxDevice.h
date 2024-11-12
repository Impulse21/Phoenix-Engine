#pragma once

#include "RHITypes.h"
#include "RHIPlatformTypes.h"

namespace phx::rhi
{
    class GfxDevice
    {
    public:
        inline static GfxDevice* Ptr = nullptr;
        static void Initialize();
        static void Finalize();

    public:
        GfxDevice();
        ~GfxDevice();

    private:
        platform::GfxDevice m_platformDevice;
    };
    
    // TODO: Create Device.
}