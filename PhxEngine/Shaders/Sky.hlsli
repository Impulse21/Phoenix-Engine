#ifndef __SKY_HLSL__
#define __SKY_HLSL__

#include "Globals.hlsli"


#define PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "CBV(b2),"  \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"


ConstantBuffer<CubemapRenderCams> CubemapRenderCamsCB : register(b2);
inline float3 GetProceduralSkyColour(in float3 viewDir)
{
    return lerp(GetHorizonColour(), GetZenithColour(), saturate(viewDir.y * 0.5f + 0.5f));
}

#endif // __SKY_HLSL__