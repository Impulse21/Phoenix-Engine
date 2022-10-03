#ifndef __GBUFFER_HLSL__
#define __GBUFFER_HLSL__

#include "Include/Shaders/ShaderInteropStructures.h"
#include "Lighting.hlsli"


#define NUM_GBUFFER_CHANNELS 3
Surface DecodeGBuffer(float4 channels[NUM_GBUFFER_CHANNELS])
{
    Surface surface = DefaultSurface();

    surface.Albedo  = channels[0].xyz;
    surface.Opactiy = channels[0].w;

    surface.Normal  = channels[1].xyz;
   
    surface.Metalness = channels[2].r;
    surface.Roughness = channels[2].g;
    surface.AO = channels[2].b;

    // TODO: Put into G Buffer;
    surface.Emissive = 0;
    surface.Specular = lerp(Fdielectric, surface.Albedo, surface.Metalness);

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