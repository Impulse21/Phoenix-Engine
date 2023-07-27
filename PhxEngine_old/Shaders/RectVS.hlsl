#pragma pack_matrix(row_major)

#include "FullScreenHelpers.hlsli"

void main(
	in uint vertexId : SV_VertexId,
	out float2 oTexCoord : TEXCOORD,
	out float4 oPositionClip : SV_Position)
{
    uint u = vertexId & 1;
    uint v = (vertexId >> 1) & 1;
	

#if true	
    CreateFullscreenTriangle_POS_UV(vertexId, oPositionClip, oTexCoord);
#else

    oPositionClip = float4(u * 2 - 1, 1 - v * 2, 0.0f, 1.0f);
    oTexCoord = float2(u, v);
#endif
}