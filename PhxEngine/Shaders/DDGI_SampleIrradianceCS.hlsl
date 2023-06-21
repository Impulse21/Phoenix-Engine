
#include "DDGI_Common.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "GBuffer_New.hlsli"

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
[numthreads(DDGI_SAMPLE_BLOCK_SIZE_X, DDGI_SAMPLE_BLOCK_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID, uint2 GroupId : SV_GroupID)
{
    const uint2 pixelPosition = DTid.xy;
    const float depth = GBuffer_Depth[pixelPosition].r;
    const float2 pixelCentre = float2(pixelPosition) + 0.5;
    const float2 texUV = pixelCentre * GetFrame().GBufferRes_RCP;
    
    
    if (depth == 1.0f)
    {
        OutputIrradianceSample[pixelPosition] = 0.0f;
        return;
    }
    
    const float3 P = ReconstructWorldPosition(GetCamera(), texUV, depth);
    const float3 N = GBuffer_1[pixelPosition].xyz;
    const float Wo = normalize(GetCamera().GetPosition() - P);
    float3 irradiance = push.GiBoost * SampleIrradiance(P, N, Wo);
    
    OutputIrradianceSample[pixelPosition] = float4(irradiance, 1.0f);
}