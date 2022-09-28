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

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
	{0.59719, 0.35458, 0.04823},
	{0.07600, 0.90834, 0.01566},
	{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367},
	{-0.10208,  1.10813, -0.00605},
	{-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

// FROM WICKED ENGINE
float3 ConvertToSRGB(float3 ldr)
{
    return ldr < 0.0031308 
		? 12.92 * ldr 
		: 1.13005 * sqrt(ldr - 0.00228) - 0.13448 * ldr + 0.005719;
}

float3 ACESFitted(float3 colour)
{
	colour = mul(ACESInputMat, colour);

	// Apply RRT and ODT
	colour = RRTAndODTFit(colour);

	colour = mul(ACESOutputMat, colour);

	// Clamp to [0, 1]
	colour = saturate(colour);

	return colour;
}

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
	const float sGamma = 2.2;
	float3 hdrColour = HdrBuffer.Sample(DefaultSampler, input.UV).rgb;

#ifdef LEGACY_TONE_MAPPING

	float3 result = hdrColour / (hdrColour + 1.0);
	// Gamma correction
	const float divGamma = 1.0 / sGamma;

	result = pow(result, float3(divGamma, divGamma, divGamma));

#else
    hdrColour *= push.Exposure;

	// -- 
	float3 result = ACESFitted(hdrColour);
	result = ConvertToSRGB(result);

#endif

	return float4(result, 1.0f);
}
#endif
#endif