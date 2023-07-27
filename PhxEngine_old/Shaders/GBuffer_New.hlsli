#ifndef __GBUFFER_HLSL__
#define __GBUFFER_HLSL__

#include "Lighting_New.hlsli"


#define NUM_GBUFFER_CHANNELS 5
Surface DecodeGBuffer(float4 channels[NUM_GBUFFER_CHANNELS])
{
    Surface surface = DefaultSurface();

    surface.Albedo  = channels[0].xyz;
    surface.Opactiy = channels[0].w;

    surface.Normal  = channels[1].xyz;

    surface.AO = channels[2].r;
    surface.Roughness = channels[2].g;
    surface.Metalness = channels[2].b;
    surface.Specular = channels[3].rgb;
    surface.Emissive = channels[4].rgb;

    return surface;
}

void EncodeGBuffer(in Surface surface, inout float4 channels[NUM_GBUFFER_CHANNELS])
{
    channels[0] = float4(surface.Albedo, 1.0f);
    channels[1] = float4(surface.Normal, 1.0f);
    channels[2] = float4(surface.AO, surface.Roughness, surface.Metalness, 1.0f);
    channels[3] = float4(lerp(Fdielectric, surface.Albedo, surface.Metalness), 1.0f);
    channels[4] = float4(surface.Emissive, 1.0f);
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