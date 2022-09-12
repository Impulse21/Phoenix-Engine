
#include "Sky.hlsli"

struct PSInput
{
	float2 ClipSpace : TEXCOORD;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{
	float4 unprojected = mul(GetCamera().ViewProjectionInv, float4(input.ClipSpace, 0.0f, 1.0f));
	unprojected /= unprojected.w;
	
	const float3 viewVector = normalize(unprojected.xyz - GetCamera().CameraPosition);

	return float4(GetProceduralSkyColour(viewVector), 1.0f);

}