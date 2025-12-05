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

float4x4 GetWorldMatrix()
{
    InstanceData *instance_buffer = g_push_constants.instance_data_ptr;
    return instance_buffer[g_push_constants.instance_index].world_matrix;
}

MaterialData GetMateralData()
{
    MaterialData *material_buffer = g_push_constants.material_data_ptr;
    return material_buffer[g_push_constants.material_index];
}

float4x4 GetViewProjMatrix()
{
    FrameData* frame_data = g_push_constants.frame_data_ptr;
    return frame_data->view_proj;
}

// --- Vertex Shader ---
[shader("vertex")]
VSOutput VertexMain(uint vertex_id : SV_VertexID)
{
    VertexInputStream stream = VertexInputStream(g_push_constants.vertex_buffer_ptr);

    float3 pos  = stream.GetPosition(vertex_id);
    float3 norm = stream.GetNormal(vertex_id);
    float4 col = stream.GetColor(vertex_id);
    float2 uv = stream.GetUV(vertex_id);

    float4x4 world_matrix = GetWorldMatrix();
    float4x4 view_project_matrix = GetViewProjMatrix();
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
    MaterialData material_data = GetMateralData();
    // Same Phong logic as before
    float3 light_dir = normalize(float3(0.5, 1.0, -0.5));
    float3 light_colour = float3(1.0, 1.0, 1.0);
    float3 view_dir = normalize(-input.world_pos);

    // Ambient
    float3 texture_colour = g_textures[material_data.base_colour_map_id].Sample(SAMPLER_LINEAR_WRAP, input.tex_coords).rgb;
    float3 colour = mul(input.colour.xyz, texture_colour);
    float3 ambient = 0.1 * colour;

    // Diffuse
    float3 N = normalize(input.normal);
    float diff = max(dot(N, light_dir), 0.0);
    float3 diffuse = diff * light_colour * input.colour.xyz;

    // Specular
    float3 reflect_dir = reflect(-light_dir, N);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
    float3 specular = float3(0.5) * spec * light_colour;

    float3 final_colour = pow(ambient + diffuse + specular, float3(1.0 / 2.2));
    return float4(final_colour, 1.0);
}