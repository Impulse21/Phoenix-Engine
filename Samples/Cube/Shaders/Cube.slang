// Cube.slang — minimal test shader for rendering a coloured cube.
//
// No vertex or index buffer is needed: the cube's 8 corners and the 36
// indices of its 12 triangles are baked in below, and the vertex shader
// looks itself up by SV_VertexID. Draw with vertex count = 36, instance
// count = 1, no bound vertex/index buffers.
//
// Expects a single push constant: a column-major view-projection * model
// matrix (mvp), matching the engine's default Slang session matrix layout.

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

struct VertexOutput
{
    float4 position : SV_Position;
    float3 colour   : COLOR0;
};

[shader("vertex")]
VertexOutput VS_Main(uint vertex_id: SV_VertexID)
{
    const int corner_index = kCubeIndices[vertex_id];
    const int face_index   = vertex_id / 6;

    VertexOutput output;
    output.position = mul(g_push_constants.mvp, float4(kCubeCorners[corner_index], 1.0));
    output.colour   = kFaceColours[face_index];
    return output;
}

[shader("fragment")]
float4 FS_Main(VertexOutput input): SV_Target
{
    return float4(input.colour, 1.0);
}
