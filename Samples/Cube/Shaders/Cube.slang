// Cube.slang — minimal test shader for rendering a coloured cube.
//
// No vertex or index buffer is needed: the cube's 8 corners and the 36
// indices of its 12 triangles are baked in below, and the vertex shader
// looks itself up by SV_VertexID. Draw with vertex count = 36, instance
// count = 1, no bound vertex/index buffers.
//
// Expects a single push constant: a model * view * projection matrix (mvp),
// laid out row-major to match hlslpp's native float4x4 layout and the
// engine's default Slang session matrix layout (see
// ShaderCompiler::InitParams::matrix_layout) — pushed as-is, no transpose.
// hlslpp's default HLSLPP_LOGICAL_LAYOUT is row-major, meaning its matrices
// are built for "vector * matrix" (row-vector-on-left) — mul() below must
// take the position first to match, not the usual mul(matrix, vector).

struct PushConstants
{
    float4x4 mvp;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_push_constants;

static const float3 kCubeCorners[8] =
{
    float3(-0.5, -0.5, -0.5), // 0
    float3( 0.5, -0.5, -0.5), // 1
    float3( 0.5,  0.5, -0.5), // 2
    float3(-0.5,  0.5, -0.5), // 3
    float3(-0.5, -0.5,  0.5), // 4
    float3( 0.5, -0.5,  0.5), // 5
    float3( 0.5,  0.5,  0.5), // 6
    float3(-0.5,  0.5,  0.5), // 7
};

// 6 faces * 2 triangles * 3 indices, wound counter-clockwise as seen from
// outside the cube. Flip the winding or disable culling if faces are
// missing — that means your rasterizer's front-face convention differs.
static const int kCubeIndices[36] =
{
    0, 1, 2,  0, 2, 3, // -Z back
    5, 4, 7,  5, 7, 6, // +Z front
    4, 0, 3,  4, 3, 7, // -X left
    1, 5, 6,  1, 6, 2, // +X right
    3, 2, 6,  3, 6, 7, // +Y top
    4, 5, 1,  4, 1, 0, // -Y bottom
};

static const float3 kFaceColours[6] =
{
    float3(1.0, 0.2, 0.2), // back   — red
    float3(0.2, 1.0, 0.2), // front  — green
    float3(0.2, 0.2, 1.0), // left   — blue
    float3(1.0, 1.0, 0.2), // right  — yellow
    float3(1.0, 0.2, 1.0), // top    — magenta
    float3(0.2, 1.0, 1.0), // bottom — cyan
};

// Flat per-face normals — same indexing as kFaceColours, hardcoded since
// this cube's geometry is baked in rather than loaded.
static const float3 kFaceNormals[6] =
{
    float3( 0.0,  0.0, -1.0), // back
    float3( 0.0,  0.0,  1.0), // front
    float3(-1.0,  0.0,  0.0), // left
    float3( 1.0,  0.0,  0.0), // right
    float3( 0.0,  1.0,  0.0), // top
    float3( 0.0, -1.0,  0.0), // bottom
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
    const int corner_index = kCubeIndices[vertex_id];
    const int face_index   = vertex_id / 6;

    VertexOutput output;
    output.position = mul(float4(kCubeCorners[corner_index], 1.0), g_push_constants.mvp);
    output.colour   = kFaceColours[face_index];
    output.normal   = kFaceNormals[face_index];
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
