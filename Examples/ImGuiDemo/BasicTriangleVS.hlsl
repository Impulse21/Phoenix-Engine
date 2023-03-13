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

[RootSignature("")]
void main(
	uint inVertexId : SV_VertexID,
	out float3 outColour : COLOR,
	out float4 outPos : SV_Position)
{
    outPos = float4(gPositions[inVertexId], 0, 1);
    outColour = gColors[inVertexId];
}
