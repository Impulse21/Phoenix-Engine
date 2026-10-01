#include "RHIVulkan.h"

using namespace phx::rhi::vulkan;

namespace phx::rhi
{
    [[nodiscard]] RenderDeviceCapabilities GetRenderDeviceCapabilities()
    {
        return g_context.capabilities;
    }

    [[nodiscard]] bool IsClipSpaceYDown()
    {
        return true;
    }

    [[nodiscard]] u64 GetFrameIndex()
    {
        return g_context.frame_number % rhi::MaxFramesInFlight;
    }

    [[nodiscard]] u64 GetFrameNumber()
    {
        return g_context.frame_number;
    }
}