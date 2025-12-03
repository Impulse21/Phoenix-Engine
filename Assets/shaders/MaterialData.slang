struct MaterialData
{
    float4 base_colour_factor;
    float4 emissive_factor;
    float4 transmission_factor;

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

[[vk::binding(0, 0)]] Texture2D g_textures[];
[[vk::binding(1, 0)]] Sampler2D g_sampled_textures[];
[[vk::binding(0, 1)]] SamplerState g_linear_sampler;