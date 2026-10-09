#pragma once

#ifdef __cplusplus
#include <PhxEngine/Renderer/Shaders/PhxInterop.h>
#else
#include "PhxInterop.h"
#endif


#ifdef __cplusplus
namespace shader_interop {
#endif

PHX_PUBLIC struct FrameData
{
    PHX_PUBLIC float4x4 view_proj;
    PHX_PUBLIC float3 camera_pos;
    PHX_PUBLIC float _padding;
    // TODO: Add light data
};

PHX_PUBLIC struct CapsuleInstanceData
{
    PHX_PUBLIC float4x4 model;
    PHX_PUBLIC float3   colour;    // flat per-entity colour (player/enemy/prop -- see HordeRenderer).
    PHX_PUBLIC float    _pad;
    PHX_PUBLIC float    radius;    // CapsuleRenderComponent::radius.
    PHX_PUBLIC float    height;    // CapsuleRenderComponent::height -- the CYLINDER's height, not including the rounded caps.
    PHX_PUBLIC float    _pad1;
    PHX_PUBLIC float    _pad2;
};

PHX_PUBLIC struct PlaneInstanceData
{
    PHX_PUBLIC float4x4 model;
    PHX_PUBLIC float3   colour;
    PHX_PUBLIC float    _pad;
    PHX_PUBLIC float2   extent;   // PlaneRenderComponent::extent -- full width (x) and depth (y, i.e. world Z).
    PHX_PUBLIC float    _pad1;
    PHX_PUBLIC float    _pad2;
};

PHX_PUBLIC struct BoxInstanceData
{
    PHX_PUBLIC float4x4 model;
    PHX_PUBLIC float3   colour;
    PHX_PUBLIC float    _pad;
    PHX_PUBLIC float3   extent;   // BoxRenderComponent::extent -- full width/height/depth.
    PHX_PUBLIC float    _pad1;
};


PHX_PUBLIC struct CapsuleDrawRoot
{
    PHX_PUBLIC FrameData*           frame_data;
    PHX_PUBLIC CapsuleInstanceData* instances;
};

PHX_PUBLIC struct PlaneDrawRoot
{
    PHX_PUBLIC FrameData*           frame_data;
    PHX_PUBLIC PlaneInstanceData*   instances;
};

PHX_PUBLIC struct BoxDrawRoot
{
    PHX_PUBLIC FrameData*           frame_data;
    PHX_PUBLIC BoxInstanceData*     instances;
};

#ifdef __cplusplus
} // namespace shader_interop;
#endif