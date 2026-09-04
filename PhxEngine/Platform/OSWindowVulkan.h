#pragma once

#include "OSWindow.h"

#include <volk.h>

namespace phx::platform::vulkan
{
    bool CreateSurface(VkInstance instance, platform::OSWindowHandle handle, VkSurfaceKHR* out_surface);
}
