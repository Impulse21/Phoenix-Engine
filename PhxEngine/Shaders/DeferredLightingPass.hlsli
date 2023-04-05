#ifndef __DEFERRED_LIGHTING_PASS_HLSL__
#define __DEFERRED_LIGHTING_PASS_HLSL__

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


#define DeferredLightingRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable(SRV(t1, numDescriptors = 6)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"

#ifdef COMPILE_CS

#define DeferredLightingRS_Compute \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=3, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable(SRV(t1, numDescriptors = 6)), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"


RWTexture2D<float4> OutputBuffer : register(u0);

// Root Constant

PUSH_CONSTANT(push, DefferedLightingCSConstants);

#endif // COMPILE_CS

Texture2D GBuffer_Depth : register(t1);
Texture2D GBuffer_0     : register(t2);
Texture2D GBuffer_1     : register(t3);
Texture2D GBuffer_2     : register(t4);
Texture2D GBuffer_3     : register(t5);
Texture2D GBuffer_4     : register(t6);

SamplerState DefaultSampler: register(s50);

struct PSInput
{
	float2 UV : TEXCOORD;
	float4 Position : SV_POSITION;
};

#ifdef COMPILE_VS
[RootSignature(DeferredLightingRS)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;

    CreateFullscreenTriangle_POS_UV(id, output.Position, output.UV);

    return output;
}

#endif

#if defined(COMPILE_PS) || defined(COMPILE_CS)

#ifdef COMPILE_CS

[RootSignature(DeferredLightingRS_Compute)]
[numthreads(DEFERRED_BLOCK_SIZE_X, DEFERRED_BLOCK_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID, uint2 GroupId : SV_GroupID)
{
#else
[RootSignature(DeferredLightingRS)]
float4 main(PSInput input) : SV_TARGET
{
    
#endif //COMPILE_CS

#ifdef COMPILE_CS
#ifdef ENABLE_THREAD_GROUP_SWIZZLING
    float2 pixelPosition = ThreadGroupTilingX(
        push.DipatchGridDim,
        uint2(DEFERRED_BLOCK_SIZE_X, DEFERRED_BLOCK_SIZE_Y),
        push.MaxTileWidth,		// User parameter (N). Recommended values: 
        GTid,		// SV_GroupThreadID
        GroupId);			// SV_GroupID

#else
    float2 pixelPosition = DTid.xy;
#endif // ENABLE_THREAD_GROUP_SWIZZLING

#else
    float2 pixelPosition = input.UV;
#endif // COMPILE_CS
    
    Camera camera = GetCamera();
    Scene scene = GetScene();

    float4 gbufferChannels[NUM_GBUFFER_CHANNELS];

#ifdef COMPILE_CS
    gbufferChannels[0] = GBuffer_0[pixelPosition];
    gbufferChannels[1] = GBuffer_1[pixelPosition];
    gbufferChannels[2] = GBuffer_2[pixelPosition];
    gbufferChannels[3] = GBuffer_3[pixelPosition];
    gbufferChannels[4] = GBuffer_4[pixelPosition];

#else
    gbufferChannels[0] = GBuffer_0.Sample(SamplerDefault, pixelPosition);
    gbufferChannels[1] = GBuffer_1.Sample(SamplerDefault, pixelPosition);
    gbufferChannels[2] = GBuffer_2.Sample(SamplerDefault, pixelPosition);
    gbufferChannels[3] = GBuffer_3.Sample(SamplerDefault, pixelPosition);
    gbufferChannels[4] = GBuffer_4.Sample(SamplerDefault, pixelPosition);
#endif

    Surface surface = DecodeGBuffer(gbufferChannels);
    const float depth = GBuffer_Depth.Sample(SamplerDefault, pixelPosition).x;
    float3 surfacePosition = ReconstructWorldPosition(camera, pixelPosition, depth);

    const float3 viewIncident = surfacePosition - (float3)GetCamera().GetPosition();
    BRDFDataPerSurface brdfSurfaceData = CreatePerSurfaceBRDFData(surface, surfacePosition, viewIncident);

    Lighting lightingTerms;
    lightingTerms.Init();

    
    if (GetFrame().Flags & FRAME_FLAGS_ENABLE_CLUSTER_LIGHTING)
    {
#ifdef COMPILE_CS
        uint2 tileIndex = uint2(floor(DTid.xy / TILE_SIZE));
#else
        uint2 tileIndex = uint2(floor(input.Position.xy / TILE_SIZE));
#endif
        uint stride = uint(NUM_WORDS) / uint(TILE_SIZE);
        uint address = tileIndex.y * stride + tileIndex.x;
        DirectLightContribution_Cluster(GetScene(), brdfSurfaceData, surface, address, lightingTerms);
    }
    else
    {
        DirectLightContribution_Simple(GetScene(), brdfSurfaceData, surface, lightingTerms);
    }
    
/*
    IndirectLightContribution_IBL(
        GetScene(),
        brdfSurfaceData,
        surface,
        ResourceHeap_GetTextureCube(GetScene().IrradianceMapTexIndex),
        ResourceHeap_GetTextureCube(GetScene().PreFilteredEnvMapTexIndex),
        ResourceHeap_GetTexture2D(FrameCB.BrdfLUTTexIndex),
        SamplerDefault,
        SamplerLinearClamped,
        lightingTerms);
*/
    IndirectLightContribution_Flat(
        0.2,
        0.0,
        lightingTerms);

	float3 finalColour = ApplyLighting(lightingTerms, brdfSurfaceData, surface);
    
#ifdef COMPILE_CS
    OutputBuffer[pixelPosition] = float4(finalColour, 1.0f);
#else

    return float4(finalColour, 1.0f);
#endif // COMPILE_CS
}
#endif // defined(COMPILE_PS) || defined(COMPILE_CS)
#endif // __DEFERRED_LIGHTING_PASS_HLSL__