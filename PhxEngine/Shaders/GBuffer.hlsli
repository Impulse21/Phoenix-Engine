#ifndef __GBUFFER_HLSL__
#define __GBUFFER_HLSL__

#include "ShaderInteropStructures.h"

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
   
    surface.Metalness = channels[2].x;
    surface.Roughness = channels[2].y;
    surface.AO = channels[2].z;

    return surface;
}

float4 ReconstructClipPosition(float2 pixelPosition, float depth)
{
    float z = depth * 2.0 - 1.0;
    return float4(pixelPosition * 2.0 - 1.0, z, 1.0);
}

float4 ReconstructViewPosition(matrix projMatrixInv, float4 clipPosition)
{
    return mul(projMatrixInv, clipPosition);
}

float3 ReconstructWorldPosition(Camera camera, float2 pixelPosition, float depth)
{
    float4 clipPosition = ReconstructClipPosition(pixelPosition, depth);
    float4 viewPosition = ReconstructViewPosition(camera.ProjInv, clipPosition);

    // Perspective division
    viewPosition /= viewPosition.w;

    float4 worldPosition = mul(camera.ViewInv, viewPosition);

    return worldPosition.xyz;
}

#endif // __GBUFFER_HLSL__