#pragma once

#include "horde_interop.h"


#ifdef __cplusplus
namespace shader_interop {
#endif

struct CapsuleInstanceData
{
    float4x4 mvp;       // model * view * projection for this entity, already combined by the caller.
    float3   colour;    // flat per-entity colour (player/enemy/prop -- see HordeRenderer).
    float    radius;    // CapsuleRenderComponent::radius.
    float    height;    // CapsuleRenderComponent::height -- the CYLINDER's height, not including the rounded caps.
};

struct PlaneInstanceData
{
    float4x4 mvp;
    float3   colour;
    float2   extent;   // PlaneRenderComponent::extent -- full width (x) and depth (y, i.e. world Z).
};

struct BoxInstanceData
{
    float4x4 mvp;
    float3   colour;
    float3   extent;   // BoxRenderComponent::extent -- full width/height/depth.
};

#ifdef __cplusplus
} // namespace shader_interop;
#endif