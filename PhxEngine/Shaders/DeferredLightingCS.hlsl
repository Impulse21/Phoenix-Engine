#pragma pack_matrix(row_major)
#define DEFERRED_LIGHTING_COMPILE_CS
// #define USE_HARD_CODED_LIGHT
// #define ENABLE_THREAD_GROUP_SWIZZLING
#include "DeferredLighting.hlsli"
/*
#define RS_Deffered \
    "RootFlags(0), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1) ),"

#define BLOCK_SIZE_X 8
#define BLOCK_SIZE_Y 8

RWTexture2D<float4> OutputBuffer : register(u0);

[RootSignature(RS_Deffered)]
[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    OutputBuffer[DTid.xy] = float4(DTid.x & DTid.y, (DTid.x & 15) / 15.0, (DTid.y & 15) / 15.0, 1.0f);
}
*/