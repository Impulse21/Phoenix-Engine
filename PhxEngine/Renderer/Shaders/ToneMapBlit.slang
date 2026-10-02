// ToneMapBlit.slang — fullscreen-triangle blit with tone mapping.

#include <StandardSamplerInterop.h>

struct PushConstants
{
    uint  scene_colour_index; // caller-resolved bindless SRV index of the blit source
    float exposure;           // linear scale applied before tonemapping
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_push_constants;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

[shader("vertex")]
VertexOutput VS_Main(uint vertex_id: SV_VertexID)
{
    VertexOutput output;
    output.uv       = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

float3 LinearToSRGB(float3 x)
{
    x = max(x, 0.0);

    return select(
        x > 0.0031308f,
        1.055f * pow(x, 1.0f / 2.4f) - 0.055f,
        12.92f * x
    );
}

// Narkowicz's ACES filmic fit — cheap, no LUT needed.
float3 ACESFilm(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("fragment")]
float4 FS_Main(VertexOutput input): SV_Target
{
    Texture2D    scene_colour = ResourceDescriptorHeap[g_push_constants.scene_colour_index];
    SamplerState linear_clamp = SamplerDescriptorHeap[StandardSampler::LinearClamp];

    float3 hdr = scene_colour.Sample(linear_clamp, input.uv).rgb;

    // Disable ACES tonemapping for now, since it's not working with prototype level.
#if 0
    hdr *= exp2(g_push_constants.exposure);
    float3 tone_mapped = ACESFilm(hdr);
#else
    float3 tone_mapped = hdr;
#endif

    float3 encoded = tone_mapped; // LinearToSRGB(tone_mapped);
    return float4(encoded, 1.0);
}

// TODO: Need to expose this on the Shader Compiler interface so we can specilice entry points
/*
[shader("fragment")]
float4 FS_Main<TToneMapper : IToneMapper>(
    VertexOutput input) : SV_Target
{
    Texture2D    scene_colour = ResourceDescriptorHeap[g_push_constants.scene_colour_index];
    SamplerState linear_clamp = SamplerDescriptorHeap[StandardSampler::LinearClamp];

    float3 hdr = scene_colour.Sample(linear_clamp, input.uv).rgb;

    // Disable ACES tonemapping for now, since it's not working with prototype level.
    hdr *= exp2(g_push_constants.exposure);

    float3 tone_mapped = TToneMapper::apply(hdr);

    float3 encoded = LinearToSRGB(tone_mapped);
    return float4(encoded, 1.0);
}
*/
