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

struct CapsuleInstanceData
{
    float4x4 model;
    float3   colour;    // flat per-entity colour (player/enemy/prop -- see HordeRenderer).
    float    _pad;
    float    radius;    // CapsuleRenderComponent::radius.
    float    height;    // CapsuleRenderComponent::height -- the CYLINDER's height, not including the rounded caps.
    float    _pad1;
    float    _pad2;
};

struct PlaneInstanceData
{
    float4x4 model;
    float3   colour;
    float    _pad;
    float2   extent;   // PlaneRenderComponent::extent -- full width (x) and depth (y, i.e. world Z).
    float    _pad1;
    float    _pad2;
};

struct BoxInstanceData
{
    float4x4 model;
    float3   colour;
    float    _pad;
    float3   extent;   // BoxRenderComponent::extent -- full width/height/depth.
    float    _pad1;
};

#ifdef __cplusplus
} // namespace shader_interop;
#endif