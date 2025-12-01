import VertexFetch;

// --- Pointers & Push Constants ---
struct PushConstants
{
    float4x4 mvp;
    float4x4 model_matrix;
    void *vertex_buffer_ptr;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push_constants;

struct VSOutput
{ 
    float4 position     : SV_Position;
    float4 colour       : COLOR;
    float3 normal       : NORMAL;
    float3 world_pos    : TEXCOORD0;
};

// --- Vertex Shader ---
[shader("vertex")]
VSOutput VertexMain(uint vertex_id : SV_VertexID)
{
    VertexInputStream stream = VertexInputStream(g_push_constants.vertex_buffer_ptr);

    float3 pos  = stream.GetPosition(vertex_id);
    float3 norm = stream.GetNormal(vertex_id);
    float4 col  = stream.GetColor(vertex_id);

    VSOutput output;
    output.position = mul(float4(pos, 1.0), g_push_constants.mvp);
    output.world_pos = mul(float4(pos, 1.0), g_push_constants.model_matrix).xyz;

    output.normal = mul(float4(norm, 0.0), g_push_constants.model_matrix).xyz;
    output.normal = normalize(output.normal); 
    
    output.colour = col;

    return output;
}

// --- Fragment (Pixel) Shader ---
[shader("fragment")]
float4 FragmentMain(VSOutput input) : SV_Target
{ 
    // Same Phong logic as before
    float3 light_dir = normalize(float3(0.5, 1.0, -0.5));
    float3 light_colour = float3(1.0, 1.0, 1.0);
    float3 view_dir = normalize(-input.world_pos);

    // Ambient
    float3 ambient = 0.1 * input.colour.xyz;

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