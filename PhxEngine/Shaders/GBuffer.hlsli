#ifndef __GBUFFER_HLSL__
#define __GBUFFER_HLSL__

#include "Include/Shaders/ShaderInteropStructures.h"

struct SurfaceProperties
{
    float3 Albedo;
    float Opactiy;
    float3 Normal;
    float Roughness;
    float Metalness;
    float AO;
};

SurfaceProperties DefaultSurfaceProperties()
{
    SurfaceProperties result;

    result.Albedo = 0;
    result.Opactiy = 1;
    result.Normal = 0;
    result.Roughness = 0;
    result.Metalness = 0;
    result.AO = 0;

    return result;
}

#define NUM_GBUFFER_CHANNELS 3
SurfaceProperties DecodeGBuffer(float4 channels[NUM_GBUFFER_CHANNELS])
{
    SurfaceProperties surface = DefaultSurfaceProperties();

    surface.Albedo  = channels[0].xyz;
    surface.Opactiy = channels[0].w;

    surface.Normal  = channels[1].xyz;
   
    surface.Metalness = channels[2].r;
    surface.Roughness = channels[2].g;
    surface.AO = channels[2].b;

    return surface;
}

float4 ReconstructClipPosition(float2 pixelPosition, float depth)
{
    return float4(pixelPosition.x * 2.0 - 1.0, 1.0 - pixelPosition.y * 2.0, depth, 1.0);
}

float4 ReconstructViewPosition(float4 clipPosition, matrix projMatrixInv)
{
    return mul(clipPosition, projMatrixInv);
}

float3 ReconstructWorldPosition(Camera camera, float2 pixelPosition, float depth)
{
    float4 clipPosition = ReconstructClipPosition(pixelPosition, depth);
    float4 unprojected = ReconstructViewPosition(clipPosition, camera.ViewProjectionInv);

    // Perspective division
    unprojected.xyz /= unprojected.w;

    return unprojected.xyz;
}

#endif // __GBUFFER_HLSL__