#pragma once

#ifdef __cplusplus
#include <PhxEngine/Renderer/Shaders/PhxInterop.h>
#else
#include "PhxInterop.h"
#endif


#ifdef __cplusplus
namespace horde::shader_interop {
#endif

PHX_PUBLIC struct RenderView
{
    PHX_PUBLIC float4x4 view;
    PHX_PUBLIC float4x4 proj;
    PHX_PUBLIC float4x4 view_proj;
};

PHX_PUBLIC struct LightDef
{
    // TODO:
};

PHX_PUBLIC struct FrameConstants
{
    PHX_PUBLIC float4x4 view_proj;
    PHX_PUBLIC float3 camera_pos;
    PHX_PUBLIC float _padding;

    RenderView* views;
    
    // TODO: Add light data
};

#ifdef __cplusplus
} // namespace shader_interop;
#endif