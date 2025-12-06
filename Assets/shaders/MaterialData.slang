struct MaterialData
{
    float4 base_colour_factor;
    float4 transmission_factor;
    float3 emissive_factor;

    float  roughness;
    float  metalness;
    float  normal_scale;
    float  alpha_cutoff;
    float  occlusion_strength;
    float  ior;
    float  specular_power;
    uint   flags;

    uint   base_colour_map_id;
    uint   normal_map_id;
    uint   material_map_id;
    uint   emissive_map_id;

    // --- Padding to 256 Bytes --
    float  _pad[40];
};

static const uint kMaxBindlessResources = 500000;
static const uint kMaxBindlessSamplers = 2048;

Texture2D<float4> g_textures_2d[kMaxBindlessResources] : register(t0, space0);
RWTexture2D<float4> g_rw_textures_2d[kMaxBindlessResources] : register(u0, space0);
SamplerState g_samplers[kMaxBindlessSamplers] : register(s0, space1);

#define SAMPLER_LINEAR_CLAMP    g_samplers[0]
#define SAMPLER_LINEAR_WRAP     g_samplers[1]
#define SAMPLER_POINT_CLAMP     g_samplers[2]
#define SAMPLER_POINT_WRAP      g_samplers[3]
#define SAMPLER_ANISO_CLAMP     g_samplers[4]
#define SAMPLER_ANISO_WRAP      g_samplers[5]
#define SAMPLER_SHADOW          g_samplers[6]