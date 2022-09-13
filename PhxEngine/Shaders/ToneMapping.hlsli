#ifndef _TONE_MAPPING_HLSL__
#define _TONE_MAPPING_HLSL__

#include "Include/Shaders/ShaderInterop.h"
#include "FullScreenHelpers.hlsli"

#if USE_RESOURCE_HEAP

#define ToneMappingRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=1, b999), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

#else

#define ToneMappingRS \
	"RootFlags(0), " \
	"RootConstants(num32BitConstants=1, b999), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

#endif

struct PushConstant
{
    float Exposure;
};

PUSH_CONSTANT(push, PushConstant);
Texture2D HdrBuffer: register(t0);
SamplerState DefaultSampler: register(s50);

struct PSInput
{
    float2 UV : TEXCOORD;
    float4 Position : SV_POSITION;
};


#ifdef TONE_MAPPING_COMPILE_VS
[RootSignature(ToneMappingRS)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;
    CreateFullscreenTriangle_POS_UV(id, output.Position, output.UV);

    return output;
}

#endif

#ifdef TONE_MAPPING_COMPILE_PS
[RootSignature(ToneMappingRS)]
float4 main(PSInput input) : SV_TARGET
{
    const float gamma = 2.2;
    float3 hdrColour = HdrBuffer.Sample(DefaultSampler, input.UV).rgb;

    
#ifdef USE_EXPOSURE
    // exposure tone mapping
    float3 mapped = float3(1.0f, 1.0f, 1.0f) - exp(-hdrColour * push.Exposure);
#else
    // reinhard tone mapping
    float3 mapped = hdrColour / (hdrColour + 1.0);
#endif 
    
    // Gamma correction
    const float divGamma = 1.0 / gamma;

    mapped = pow(mapped, float3(divGamma, divGamma, divGamma));
    return float4(mapped, 1.0f);
}
#endif
#endif