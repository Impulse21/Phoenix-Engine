#ifndef __SKY_HLSL__
#define __SKY_HLSL__

inline float3 GetProceduralSkyColour(in float3 viewDir)
{
    float3 skyColour = 0.0f;

    skyColour = lerp(GetHorizonColour(), GetZenithColour(), saturate(viewDir.y * 0.5f + 0.5f));

    // Calulate exposure?
    return skyColour;
}

#endif // __SKY_HLSL__