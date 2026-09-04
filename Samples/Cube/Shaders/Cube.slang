// Cube.slang — minimal test shader for rendering a coloured cube.
//
// Vertex/index/normal data lives in real GPU buffers (see CubeApp::OnInit),
// read here through buffer-device-address pointers carried in the push
// constants — no vertex/index buffer bound at the Vulkan level at all, the
// vertex shader just dereferences g_push_constants.indices/vertices itself,
// indexed by SV_VertexID. Draw with vertex count = 36, instance count = 1.
//
// Expects a single push constant: two BDA pointers (vertices, indices) plus
// a model * view * projection matrix (mvp), laid out row-major to match
// hlslpp's native float4x4 layout and the engine's default Slang session
// matrix layout (see ShaderCompiler::InitParams::matrix_layout) — pushed
// as-is, no transpose. hlslpp's default HLSLPP_LOGICAL_LAYOUT is row-major,
// meaning its matrices are built for "vector * matrix" (row-vector-on-left)
// — mul() below must take the position first to match, not the usual
// mul(matrix, vector).

struct Vertex
{
    float3 position;
    float3 normal;
};

struct PushConstants
{
    Vertex*   vertices;
    uint32_t* indices;
    float4x4  mvp;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_push_constants;

static const float3 kFaceColours[6] =
{
    float3(1.0, 0.2, 0.2), // back   — red
    float3(0.2, 1.0, 0.2), // front  — green
    float3(0.2, 0.2, 1.0), // left   — blue
    float3(1.0, 1.0, 0.2), // right  — yellow
    float3(1.0, 0.2, 1.0), // top    — magenta
    float3(0.2, 1.0, 1.0), // bottom — cyan
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 colour   : COLOR0;
    float3 normal   : NORMAL0;
};

[shader("vertex")]
VertexOutput VS_Main(uint vertex_id: SV_VertexID)
{
    const uint corner_index = g_push_constants.indices[vertex_id];
    const uint face_index   = vertex_id / 6;
    Vertex v = g_push_constants.vertices[corner_index];

    VertexOutput output;
    output.position = mul(float4(v.position, 1.0), g_push_constants.mvp);
    output.colour   = kFaceColours[face_index];
    output.normal   = v.normal;
    return output;
}

[shader("fragment")]
float4 FS_Main(VertexOutput input): SV_Target
{
    // Phong logic as before
    float3 light_dir = normalize(float3(0.5, 1.0, -0.5));
    float3 light_colour = float3(1.0, 1.0, 1.0);
    float3 view_dir = normalize(float3(0.0, -1.0, 0.0));

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
