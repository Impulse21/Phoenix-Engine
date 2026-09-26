// ToneMapBlit.slang — fullscreen-triangle blit with tone mapping.
//
// Draws the linear HDR scene colour target into the final present target,
// applying an ACES filmic tonemap + gamma encode along the way. No
// vertex/index buffer needed: the triangle covers the screen from 3
// procedural vertices (SV_VertexID 0..2), same trick as Cube.slang.
//
// Uses the VK_EXT_descriptor_heap bindless model — no [[vk::binding]]
// descriptor sets at all (see Cube.slang). This pass owns no descriptor
// state of its own: the caller has already bound its resource/sampler
// heaps via CmdSetDescriptorHeaps before calling ToneMapBlit::Blit, and
// hands in the source texture's index (already written into that heap).

struct PushConstants
{
    uint  scene_colour_index; // caller-resolved bindless SRV index of the blit source
    float exposure;           // linear scale applied before tonemapping
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_push_constants;

// Index of a Linear-Clamp sampler in the caller's shared sampler heap —
// by convention slot 0 (see StandardSampler::LinearClamp).
static const uint kSamplerLinearClamp = 0;

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
    SamplerState linear_clamp = SamplerDescriptorHeap[kSamplerLinearClamp];

    float3 hdr         = scene_colour.Sample(linear_clamp, input.uv).rgb * g_push_constants.exposure;
    float3 tone_mapped = ACESFilm(hdr);

    // The present target is a plain UNORM format, not an sRGB view, so
    // the shader is responsible for display (gamma) encoding.
    float3 encoded = pow(tone_mapped, 1.0 / 2.2);
    return float4(encoded, 1.0);
}
