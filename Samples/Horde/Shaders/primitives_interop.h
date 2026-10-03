#pragma once

#ifdef __cplusplus
#include <PhxEngine/Renderer/Shaders/PhxInterop.h>
#else
#include "PhxInterop.h"
#endif


#ifdef __cplusplus
namespace shader_interop {
#endif

struct CapsuleInstanceData
{
    float4x4 mvp;       // model * view * projection for this entity, already combined by the caller.
    float3   colour;    // flat per-entity colour (player/enemy/prop -- see HordeRenderer).
    float    _pad;
    float    radius;    // CapsuleRenderComponent::radius.
    float    height;    // CapsuleRenderComponent::height -- the CYLINDER's height, not including the rounded caps.
    float    _pad1;
    float    _pad2;
};

struct PlaneInstanceData
{
    float4x4 mvp;
    float3   colour;
    float    _pad;
    float2   extent;   // PlaneRenderComponent::extent -- full width (x) and depth (y, i.e. world Z).
    float    _pad1;
    float    _pad2;
};

struct BoxInstanceData
{
    float4x4 mvp;
    float3   colour;
    float    _pad;
    float3   extent;   // BoxRenderComponent::extent -- full width/height/depth.
    float    _pad1;
};

#ifdef __cplusplus
} // namespace shader_interop;
#endif