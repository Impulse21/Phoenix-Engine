#pragma pack_matrix(row_major)

#include "Sky.hlsli"

struct PSInput
{
	float2 ClipSpace : TEXCOORD;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{
	float4 unprojected = mul(float4(input.ClipSpace, 0.0f, 1.0f), GetCamera().ViewProjectionInv);
	unprojected.xyz /= unprojected.w;

    const float3 V = normalize(unprojected.xyz - GetCamera().GetPosition());
	float3 skyColour = GetProceduralSkyColour(V);

	return float4(skyColour, 1.0f);

}