
#include "RHIVulkan.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StaticArray.h>

using namespace phx;
using namespace phx::rhi;

bool phx::rhi::BeginFrame(ViewportHandle viewport)
{
    PHX_UNUSED(viewport);
    return true;
}

bool phx::rhi::EndFrame(ViewportHandle viewport)
{
    PHX_UNUSED(viewport);
    return true;
}