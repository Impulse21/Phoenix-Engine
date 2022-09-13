
#include "Sky.hlsli"

struct PSInput
{
	float2 ClipSpace : TEXCOORD;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{
	float4 viewPos = mul(GetCamera().ProjInv, float4(input.ClipSpace, 1.0f, 1.0f));
	viewPos /= viewPos.w;

	float4 worldPos = mul(GetCamera().ViewInv, viewPos);
	const float3 viewVector = normalize(worldPos.xyz - GetCamera().CameraPosition);

	float3 skyColour = GetProceduralSkyColour(viewVector);

	return float4(skyColour, 1.0f);

}