#define BLIT_ROOTSIGNATURE \
	"DescriptorTable(SRV(t0, numDescriptors = 1)), " \
	"StaticSampler(s0, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"


[RootSignature(BLIT_ROOTSIGNATURE)]
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