
#include "Globals_New.hlsli"
#include "DDGI_Common.hlsli"
#include "ResourceHeapTables.hlsli"

struct PSInput
{
    float3 Normal : NORMAL;
    uint ProbeIndex : PROBEINDEX;
    float4 Position : SV_Position;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{
    const uint3 probeCoord = DDGI_GetProbeCoord(input.ProbeIndex);
    
    float3 colour = ResourceHeap_GetTexture2D(GetScene().DDGI.IrradianceAtlasTextureIdPrev).SampleLevel(SamplerLinearClamped, ProbeColourUV(probeCoord, input.Normal), 0).rgb;

    return float4(colour, 1);
}
