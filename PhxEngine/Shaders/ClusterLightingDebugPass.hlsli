#ifndef __CLUSTER_LIGHTING_DEBUG_PASS__
#define __CLUSTER_LIGHTING_DEBUG_PASS__

#include "Globals_New.hlsli"
#include "Lighting_New.hlsli"
#include "GBuffer_New.hlsli"
#include "FullScreenHelpers.hlsli"
#include "Shadows.hlsli"

// Helpers

//#define COMPILE_VS
//#define COMPILE_PS
//#define COMPILE_CS

#ifdef ENABLE_THREAD_GROUP_SWIZZLING
#include "ThreadGroupTilingX.hlsli"
#endif


#define ClusterLightingDebugRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), "

#ifdef COMPILE_CS

#define ClusterLightingDebugRS_Compute \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=3, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1) ),"

RWTexture2D<float4> OutputBuffer : register(u0);

#endif

struct PSInput
{
    float2 UV : TEXCOORD;
    float4 Position : SV_POSITION;
};

#ifdef COMPILE_VS
[RootSignature(ClusterLightingDebugRS)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;

    CreateFullscreenTriangle_POS_UV(id, output.Position, output.UV);

    return output;
}

#endif

#if defined(COMPILE_PS) || defined(COMPILE_CS)

float3 Geoffrey(float t)
{
    float3 r = t * 2.1 - float3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}

#ifdef COMPILE_CS

[RootSignature(ClusterLightingDebugRS_Compute)]
[numthreads(DEFERRED_BLOCK_SIZE_X, DEFERRED_BLOCK_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID, uint2 GroupId : SV_GroupID)
{
#else
[RootSignature(ClusterLightingDebugRS)]
float4 main(PSInput input) : SV_TARGET
{
    
#endif //COMPILE_CS
    
    Camera camera = GetCamera();
    Scene scene = GetScene();
    
#ifdef COMPILE_CS
        uint2 tileIndex = uint2(floor(DTid.xy / TILE_SIZE));
#else
    uint2 tileIndex = uint2(floor(input.Position.xy / TILE_SIZE));
#endif
    uint stride = uint(NUM_WORDS) / uint(TILE_SIZE);
    uint tileAddress = tileIndex.y * stride + tileIndex.x;
    
    uint lightCount = 0;
    [loop]
    for (int iBin = 0; iBin < NUM_BINS; iBin++)
    {
        const uint binValue = GetLightBin(iBin);
        const uint minLightId = binValue & 0xFFFF;
        const uint maxLightId = (binValue >> 16) & 0xFFFF;
        if (minLightId != NUM_LIGHTS + 1)
        {
            for (uint lightID = minLightId; lightID <= maxLightId; ++lightID)
            {
                uint wordId = lightID / 32;
                uint bitId = lightID % 32;

                uint mask = GetLightTile(tileAddress + wordId);
                mask &= 0xFFFFFFFF;
                
                while (mask != 0)
                {
                    uint bitIndex = firstbitlow(mask);
                    mask ^= (1UL << bitIndex);

                    lightCount++;
                }
            }
        }
    }
    
    float alpha = lightCount > 0 ? 1.0 : 0.0;
    float4 finalColour = float4(Geoffrey(saturate(lightCount / 16.0)), alpha);
    
#ifdef COMPILE_CS
    OutputBuffer[DTid.xy] = finalColour;
#else

    return finalColour;
#endif // COMPILE_CS
}
#endif // defined(COMPILE_PS) || defined(COMPILE_CS)
#endif // __CLUSTER_LIGHTING_DEBUG_PASS__