#pragma once

namespace phx::rhi::vulkan
{

}


#define vulkan_check(call) [&]() { VkResult res = call; PHX_ASSERT(res >= VK_SUCCESS); return res; }()