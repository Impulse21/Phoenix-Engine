

#include "Include/Shaders/ShaderInterop.h"
#include "Include/Shaders/ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"


// Credit -> Wicked Engine
// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// uv			: UV texture coordinates on cubemap face in range [0, 1]
// faceIndex	: cubemap face index as in the backing texture2DArray in range [0, 5]
// returns direction to be used in cubemap sampling
inline float3 UvToCubeMap(in float2 uv, in uint faceIndex)
{
	// get uv in [-1, 1] range:
	uv = uv * 2 - 1;

	// and UV.y should point upwards:
	uv.y *= -1;

	switch (faceIndex)
	{
	case 0:
		// +X
		return float3(1, uv.y, -uv.x);
	case 1:
		// -X
		return float3(-1, uv.yx);
	case 2:
		// +Y
		return float3(uv.x, 1, -uv.y);
	case 3:
		// -Y
		return float3(uv.x, -1, uv.y);
	case 4:
		// +Z
		return float3(uv, 1);
	case 5:
		// -Z
		return float3(-uv.x, uv.y, -1);
	default:
		// error
		return 0;
	}
}

#define GENERATE_MIP_CHAIN_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=12, b999), " \
    "StaticSampler(s0," \
           "addressU = TEXTURE_ADDRESS_CLAMP," \
           "addressV = TEXTURE_ADDRESS_CLAMP," \
           "addressW = TEXTURE_ADDRESS_CLAMP," \
           "filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState SamplerLinearClamp : register(s0);

PUSH_CONSTANT(push, GenerateMipChainPushConstants);

[RootSignature(GENERATE_MIP_CHAIN_ROOTSIGNATURE)]
[numthreads(GENERATE_MIP_CHAIN_2D_BLOCK_SIZE, GENERATE_MIP_CHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < push.OutputResolution.x && DTid.y < push.OutputResolution.y)
    {
        TextureCubeArray input = ResourceHeap_GetTextureCubeArray(push.TextureInput);
        RWTexture2DArray<float4> output = ResourceHeap_GetRWTexture2DArray(push.TextureOutput);

        float2 uv = ((float2) DTid.xy + 0.5f) * push.OutputResolutionRcp.xy;
        float3 N = UvToCubeMap(uv, DTid.z);

        output[uint3(DTid.xy, DTid.z + push.ArrayIndex * 6)] = input.SampleLevel(SamplerLinearClamp, float4(N, push.ArrayIndex), 0);
    }
}