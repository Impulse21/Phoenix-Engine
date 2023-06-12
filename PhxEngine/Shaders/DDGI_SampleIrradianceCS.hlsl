
#include "DDGI_Common.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "GBuffer_New.hlsli"

#define THREAD_COUNT 32
#define RS_DDGI_UPDATE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=20, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable(SRV(t1, numDescriptors = 6)), " \
    "DescriptorTable( UAV(u0, numDescriptors = 2) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

PUSH_CONSTANT(push, DDGIPushConstants);
RWTexture2D<float4> OutputIrradianceSample : register(u0);


Texture2D GBuffer_Depth : register(t1);
Texture2D GBuffer_0 : register(t2);
Texture2D GBuffer_1 : register(t3); // Normal Buffer
Texture2D GBuffer_2 : register(t4);
Texture2D GBuffer_3 : register(t5);
Texture2D GBuffer_4 : register(t6);

[RootSignature(RS_DDGI_UPDATE)]
[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID, uint2 GroupId : SV_GroupID)
{
    float2 pixelPosition = DTid.xy;
    const float depth = GBuffer_Depth.Sample(SamplerDefault, pixelPosition).x;
    
    if (depth == 1.0f)
    {
        OutputIrradianceSample[pixelPosition] = 0.0f;
    }
    
    const float3 P = ReconstructWorldPosition(GetCamera(), pixelPosition, depth);
    const float3 N = GBuffer_1[pixelPosition].xyz;
    
    float3 irradiance = push.GiBoost * SampleIrradiance(P, N);

    // Store
    OutputIrradianceSample[pixelPosition] = float4(irradiance, 1.0f);
}