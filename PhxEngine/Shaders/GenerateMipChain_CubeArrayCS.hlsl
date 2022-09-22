

#include "Include/Shaders/ShaderInterop.h"
#include "Include/Shaders/ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"
#include "CommonFunctions.hlsli"

#define GENERATE_MIP_CHAIN_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=8, b999), " \
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