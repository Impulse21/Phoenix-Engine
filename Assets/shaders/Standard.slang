import VertexFetch;

#include "MaterialData.slang"

struct InstanceData
{
    float4x4 world_matrix;
    float4x4 world_matrix_inverse;
};

struct FrameData
{
    float4x4 view_proj;
    float3 camera_pos;
    float time;
};

struct PushConstants
{
    FrameData*      frame_data_ptr;
    InstanceData*   instance_data_ptr;
    MaterialData*   material_data_ptr;
    void*           vertex_buffer_ptr;

    uint material_index;
    uint instance_index;

    // -- Padding of 128 ---
    uint32_t _padding[22];
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push_constants;

struct VSOutput
{ 
    float4 position     : SV_Position;
    float4 colour       : COLOR0;
    float3 normal       : NORMAL;
    float2 tex_coords   : TEXCOORD0;
    float3 world_pos    : COLOR1;
};


// --- Vertex Shader ---
[shader("vertex")]
VSOutput VertexMain(uint vertex_id: SV_VertexID)
{ 
    InstanceData *instances = g_push_constants.instance_data_ptr;
    FrameData *frame = g_push_constants.frame_data_ptr;
    VertexInputStream stream = VertexInputStream(g_push_constants.vertex_buffer_ptr);

    float3 pos  = stream.GetPosition(vertex_id);
    float3 norm = stream.GetNormal(vertex_id);
    float4 col = stream.GetColor(vertex_id);
    float2 uv = stream.GetUV(vertex_id);

    float4x4 world_matrix = instances[g_push_constants.instance_index].world_matrix;
    float4x4 view_project_matrix = frame->view_proj;

    float4x4 world_to_projection = mul(world_matrix, view_project_matrix);

    VSOutput output;
    output.position = mul(float4(pos, 1.0), world_to_projection);
    output.world_pos = mul(float4(pos, 1.0), world_matrix).xyz;

    output.normal = mul(float4(norm, 0.0), world_matrix).xyz;
    output.normal = normalize(output.normal);
    output.tex_coords = uv;
    output.colour = col;

    return output;
}

// --- Fragment (Pixel) Shader ---
[shader("fragment")]
float4 FragmentMain(VSOutput input) : SV_Target
{
    uint64_t ptrAddress = (uint64_t)g_push_constants.material_data_ptr;

    // If pointer is 0, turn the screen MAGENTA
    if (ptrAddress == 0)
    {
        return float4(1, 0, 1, 1);
    }
    MaterialData *material_data = &g_push_constants.material_data_ptr[g_push_constants.material_index];

    // 1. Calculate Normal early (needed for both Diffuse and Indirect/Ambient)
    float3 N = normalize(input.normal);

    float3 light_dir = normalize(float3(0.5, 1.0, -0.5));
    float3 light_colour = float3(1.0, 1.0, 1.0);
    float3 view_dir = normalize(-input.world_pos);

    // Texture Sampling
    float3 texture_colour = float3(1.0f);
    if (material_data->base_color_texture != ~0ul)
    {
        texture_colour = g_textures_2d[material_data->base_color_texture].Sample(
                                                                             SAMPLER_LINEAR_WRAP,
                                                                             input.tex_coords
        ) .rgb;
    }

    float3 base_colour_factor = material_data.base_colour_factor.rgb;
    float3 colour = mul(base_colour_factor, texture_colour);

    // --- INDIRECT LIGHT (Hemispheric Ambient) ---
    // Instead of flat 0.1, we blend between a sky color and ground color based on Normal Y.
    // This gives shape to the shadows.
    float3 sky_color = float3(0.2, 0.24, 0.35);     // Soft Blue-Grey (Sky)
    float3 ground_color = float3(0.05, 0.04, 0.04); // Dark Warm Grey (Ground/Bounce)

    // Remap Normal Y from [-1, 1] to [0, 1] for blending
    float hemi_mix = N.y * 0.5 + 0.5;
    float3 ambient_light = lerp(ground_color, sky_color, hemi_mix);

    // Apply the indirect light to the material color
    float3 ambient = ambient_light * colour;

    // --- DIFFUSE ---
    float diff = max(dot(N, light_dir), 0.0);
    float3 diffuse = diff * light_colour * colour;

    // --- SPECULAR ---
    float3 reflect_dir = reflect(-light_dir, N);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
    float3 specular = float3(0.5) * spec * light_colour;

    // Combine
    float3 final_colour = pow(ambient + diffuse + specular, float3(1.0 / 2.2));
    return float4(final_colour, 1.0);
}