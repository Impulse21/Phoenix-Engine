#include "RHIVulkan.h"

using namespace phx::rhi::vulkan;

namespace phx::rhi
{
    [[nodiscard]] DeviceCapabilities GetDeviceCapabilities()
    {
        return g_context.capabilities;
    }

    [[nodiscard]] bool IsClipSpaceYDown()
    {
        return true;
    }
}