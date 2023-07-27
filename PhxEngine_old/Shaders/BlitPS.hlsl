#pragma pack_matrix(row_major)

#define BLIT_ROOTSIGNATURE \
	"DescriptorTable(SRV(t0, numDescriptors = 1)), " \
	"StaticSampler(s0, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

Texture2D SourceTexture : register(t0);
SamplerState Sampler : register(S0);

[RootSignature(BLIT_ROOTSIGNATURE)]
float4 main(
	in float2 texCoord : TEXCOORD,
	in float4 positionClip : SV_Position) : SV_TARGET
{
    return SourceTexture.Sample(Sampler, texCoord);
}