
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
    // TODO: Debug Flag
    const bool kHighlightOffsetProbes = false;
    const uint3 probeCoord = DDGI_GetProbeCoord(input.ProbeIndex);
    const float2 uv = ProbeColourUV(probeCoord, input.Normal);
    float3 colour = ResourceHeap_GetTexture2D(GetScene().DDGI.IrradianceAtlasTextureIdPrev).SampleLevel(SamplerLinearClamped, uv, 0).rgb;

    [branch]
    if (kHighlightOffsetProbes)
    {
        if (GetScene().DDGI.FrameIndex > 0)
        {
            if (GetScene().DDGI.OffsetBufferId >= 0)
            {
                float3 currentOffset = ResourceHeap_GetBuffer(GetScene().DDGI.OffsetBufferId).Load < DDGIProbeOffset > (input.ProbeIndex * sizeof(DDGIProbeOffset)).load();
                if (any(currentOffset))
                {
                    colour = float3(1.0f, 0.0f, 0.0f);
                }
                else
                {
                    colour = float3(0.0f, 0.0f, 1.0f);
                }
            }
        }
    }
    
    return float4(colour, 1);
}
