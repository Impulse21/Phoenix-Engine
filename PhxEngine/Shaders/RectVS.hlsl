

void main(
	in uint vertexId : SV_VertexId,
	out float4 oPositionClip : SV_Position,
	out float2 oTexCoord : TEXCOORD )
{
    uint u = vertexId & 1;
    uint v = (vertexId >> 1) & 1;
	
    oPositionClip = float4(u * 2 - 1, 1 - v * 2, 0.0f, 1.0f);
    oTexCoord = float2(u, v);
}