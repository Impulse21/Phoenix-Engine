// ToneMapBlit.slang — fullscreen-triangle blit with tone mapping.
//
// Draws the linear HDR scene colour target into the final present target,
// applying an ACES filmic tonemap + gamma encode along the way. No
// vertex/index buffer needed: the triangle covers the screen from 3
// procedural vertices (SV_VertexID 0..2), same trick as Cube.slang.
//
// Binding layout matches the engine's single global bindless pipeline
// layout (see VulkanDescriptorSystem.cpp): set 0 / binding 0 is the
// mutable bindless resource heap, set 1 / binding 0 is the fixed global
// sampler array. The source texture and sampler are selected by index
// via push constants rather than a per-draw descriptor set.

struct PushConstants
{
    uint  scene_colour_index; // bindless SRV index — VulkanTexture::srv_index
    float exposure;           // linear scale applied before tonemapping
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_push_constants;

[[vk::binding(0, 0)]] Texture2D    g_textures[];
[[vk::binding(0, 1)]] SamplerState g_samplers[];

// Index into the engine's fixed global sampler array — order defined by
// VulkanDescriptorSystem::CreateGlobalSamplers. 0 == Linear Clamp.
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
    Texture2D    scene_colour = g_textures[NonUniformResourceIndex(g_push_constants.scene_colour_index)];
    SamplerState linear_clamp = g_samplers[kSamplerLinearClamp];

    float3 hdr         = scene_colour.Sample(linear_clamp, input.uv).rgb * g_push_constants.exposure;
    float3 tone_mapped = ACESFilm(hdr);

    // The present target is a plain UNORM format, not an sRGB view, so
    // the shader is responsible for display (gamma) encoding.
    float3 encoded = pow(tone_mapped, 1.0 / 2.2);
    return float4(encoded, 1.0);
}
