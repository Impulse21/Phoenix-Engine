
#include "Sky.hlsli"

struct PSInput
{
	float2 ClipSpace : TEXCOORD;
};

PUSH_CONSTANT(push, ImagePassPushConstants);

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{
    float4 unprojected = mul(float4(input.ClipSpace, 0.0f, 1.0f), GetCamera().ViewProjectionInv);
    unprojected.xyz /= unprojected.w;

    const float3 V = normalize(unprojected.xyz - GetCamera().CameraPosition);
    float3 envColour = ResourceHeap_GetTextureCube(push.Index).Sample(SamplerLinearClamped, V).rgb;

    return float4(envColour, 1.0f);

}