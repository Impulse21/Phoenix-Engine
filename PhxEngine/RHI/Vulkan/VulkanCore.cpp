#pragma once

#include "RHIVulkan.h"
#include <PhxEngine/RHI/RHI.h>


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>


bool phx::rhi::Initialize()
{
    return true;
}

void phx::rhi::Shutdown()
{

}
