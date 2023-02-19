
#define MAX_PRIMS_PER_MESHLET 1
#define MAX_VERTICES_PER_MESHLET 3

static const float2 gPositions[] =
{
    float2(-0.5, -0.5),
	float2(0, 0.5),
	float2(0.5, -0.5)
};

static const float3 gColors[] =
{
    float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1)
};

struct Payload
{
    int Dummy;
};

struct VertexOutput
{
    float3 Colour : COLOR;
    float4 Position : SV_Position;
};

groupshared Payload s_payload;

[RootSignature("")]
[numthreads(1, 1, 1)]
[outputtopology("triangle")]
void main(
    uint threadId : SV_GroupThreadID,
    in payload Payload i_payload,
    out indices uint3 o_tris[MAX_PRIMS_PER_MESHLET],
    out vertices VertexOutput o_verts[MAX_VERTICES_PER_MESHLET])
{
    SetMeshOutputCounts(3, 1);
    o_tris[0] = uint3(0, 1, 2);

    for (uint i = 0; i < 3; i++)
    {
        o_verts[i].Position = float4(gPositions[i], 0, 1);
        o_verts[i].Colour = gColors[i];
    }
}
