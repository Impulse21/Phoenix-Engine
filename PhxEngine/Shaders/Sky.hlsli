#ifndef __SKY_HLSL__
#define __SKY_HLSL__

#include "Globals.hlsli"

inline float3 GetProceduralSkyColour(in float3 viewDir)
{
    return lerp(GetHorizonColour(), GetZenithColour(), saturate(viewDir.y * 0.5f + 0.5f));
}

#endif // __SKY_HLSL__