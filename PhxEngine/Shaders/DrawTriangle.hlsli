#ifndef __FULLSCREEN_QUAD_HLSL__
#define __FULLSCREEN_QUAD_HLSL__

#include "FullScreenHelpers.hlsli"

// TODO: ROOT SIGNATURE
#define FullScreenRS \
	"RootFlags(0), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

Texture2D Texture: register(t0);
SamplerState DefaultSampler: register(s50);

struct PSInput
{
	float2 UV : TEXCOORD;
	float4 Position : SV_POSITION;
};

#ifdef FULLSCREEN_QUAD_COMPILE_VS
[RootSignature(FullScreenRS)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;
    // TODO: Add share
    CreateFullscreenTriangle_POS_UV(id, output.Position, output.UV);

    return output;
}

#endif

#ifdef FULLSCREEN_QUAD_COMPILE_PS
[RootSignature(FullScreenRS)]
float4 main(PSInput input) : SV_TARGET
{ 
    // return float4(1.0f, 0.0f, 0.0f, 1.0f);
    // return float4(input.UV, 0.0f, 1.0f);
    // TODO: Need to linearize Depth to display it correctly.
    return float4(Texture.Sample(DefaultSampler, input.UV).rgb, 1.0f);
}
#endif
#endif // __FULLSCREEN_QUAD_HLSL__