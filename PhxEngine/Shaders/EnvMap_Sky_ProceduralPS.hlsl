#pragma pack_matrix(row_major)

#include "Sky.hlsli"

struct PSInput
{
	float3 NormalWS     : NORMAL;
};

[RootSignature(PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{

	float3 normal = normalize(input.NormalWS);
	float4 colour = float4(GetProceduralSkyColour(input.NormalWS, false), 1);
	return colour;
}
