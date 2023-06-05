
#include "Globals_New.hlsli"

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
    
    // TODO: Use the radiance results as it's colour, which isn't working right now.
    // Texture2D ddgiColorTexture = bindless_textures[GetScene().ddgi.color_texture];
    // float3 color = ddgiColorTexture.SampleLevel(sampler_linear_clamp, ddgi_probe_color_uv(probeCoord, input.normal), 0).rgb;
    float3 colour = float3(
            float(input.ProbeIndex & 1),
            float(input.ProbeIndex & 3) / 4,
            float(input.ProbeIndex & 7) / 8);

    if (colour.r == 0 && colour.g == 0 && colour.b == 0)
    {
        colour = 0.4;
    }
    
    return float4(colour, 1);
}