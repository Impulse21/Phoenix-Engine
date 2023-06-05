#ifndef __DDGI_UPDATE_HLSLI__
#define __DDGI_UPDATE_HLSLI__

#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "Defines.hlsli"


#ifdef DDGI_UPDATE_DEPTH
#define THREAD_COUNT DDGI_DEPTH_RESOLUTION
#else
#define THREAD_COUNT DDGI_COLOUR_RESOLUTION
#endif

#define RS_DDGI_UPDATE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=16, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 2) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

PUSH_CONSTANT(push, DDGIPushConstants);
RWTexture2D<float4> OutputIrradianceAtlas : register(u0);
RWTexture2D<float2> OutputVisibilityAtlas : register(u1);

inline uint2 ProbeColourPixel(uint3 probeCoord)
{
    return probeCoord.xz * DDGI_COLOUR_TEXELS + uint2(probeCoord.y * GetScene().DDGI.GridDimensions.x * DDGI_COLOUR_TEXELS, 0) + 1;
}

inline uint2 ProbeDepthPixel(uint3 probeCoord)
{
    return probeCoord.xz * DDGI_DEPTH_TEXELS + uint2(probeCoord.y * GetScene().DDGI.GridDimensions.x * DDGI_DEPTH_TEXELS, 0) + 1;
}


// Border offsets from: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_border_update.glsl
#ifdef DDGI_UPDATE_DEPTH
static const uint4 DDGI_DEPTH_BORDER_OFFSETS[68] =
{
    uint4(16, 1, 1, 0),
	uint4(15, 1, 2, 0),
	uint4(14, 1, 3, 0),
	uint4(13, 1, 4, 0),
	uint4(12, 1, 5, 0),
	uint4(11, 1, 6, 0),
	uint4(10, 1, 7, 0),
	uint4(9, 1, 8, 0),
	uint4(8, 1, 9, 0),
	uint4(7, 1, 10, 0),
	uint4(6, 1, 11, 0),
	uint4(5, 1, 12, 0),
	uint4(4, 1, 13, 0),
	uint4(3, 1, 14, 0),
	uint4(2, 1, 15, 0),
	uint4(1, 1, 16, 0),
	uint4(16, 16, 1, 17),
	uint4(15, 16, 2, 17),
	uint4(14, 16, 3, 17),
	uint4(13, 16, 4, 17),
	uint4(12, 16, 5, 17),
	uint4(11, 16, 6, 17),
	uint4(10, 16, 7, 17),
	uint4(9, 16, 8, 17),
	uint4(8, 16, 9, 17),
	uint4(7, 16, 10, 17),
	uint4(6, 16, 11, 17),
	uint4(5, 16, 12, 17),
	uint4(4, 16, 13, 17),
	uint4(3, 16, 14, 17),
	uint4(2, 16, 15, 17),
	uint4(1, 16, 16, 17),
	uint4(1, 16, 0, 1),
	uint4(1, 15, 0, 2),
	uint4(1, 14, 0, 3),
	uint4(1, 13, 0, 4),
	uint4(1, 12, 0, 5),
	uint4(1, 11, 0, 6),
	uint4(1, 10, 0, 7),
	uint4(1, 9, 0, 8),
	uint4(1, 8, 0, 9),
	uint4(1, 7, 0, 10),
	uint4(1, 6, 0, 11),
	uint4(1, 5, 0, 12),
	uint4(1, 4, 0, 13),
	uint4(1, 3, 0, 14),
	uint4(1, 2, 0, 15),
	uint4(1, 1, 0, 16),
	uint4(16, 16, 17, 1),
	uint4(16, 15, 17, 2),
	uint4(16, 14, 17, 3),
	uint4(16, 13, 17, 4),
	uint4(16, 12, 17, 5),
	uint4(16, 11, 17, 6),
	uint4(16, 10, 17, 7),
	uint4(16, 9, 17, 8),
	uint4(16, 8, 17, 9),
	uint4(16, 7, 17, 10),
	uint4(16, 6, 17, 11),
	uint4(16, 5, 17, 12),
	uint4(16, 4, 17, 13),
	uint4(16, 3, 17, 14),
	uint4(16, 2, 17, 15),
	uint4(16, 1, 17, 16),
	uint4(16, 16, 0, 0),
	uint4(1, 16, 17, 0),
	uint4(16, 1, 0, 17),
	uint4(1, 1, 17, 17)
};
#else
static const uint4 DDGI_COLOR_BORDER_OFFSETS[36] =
{
    uint4(8, 1, 1, 0),
	uint4(7, 1, 2, 0),
	uint4(6, 1, 3, 0),
	uint4(5, 1, 4, 0),
	uint4(4, 1, 5, 0),
	uint4(3, 1, 6, 0),
	uint4(2, 1, 7, 0),
	uint4(1, 1, 8, 0),
	uint4(8, 8, 1, 9),
	uint4(7, 8, 2, 9),
	uint4(6, 8, 3, 9),
	uint4(5, 8, 4, 9),
	uint4(4, 8, 5, 9),
	uint4(3, 8, 6, 9),
	uint4(2, 8, 7, 9),
	uint4(1, 8, 8, 9),
	uint4(1, 8, 0, 1),
	uint4(1, 7, 0, 2),
	uint4(1, 6, 0, 3),
	uint4(1, 5, 0, 4),
	uint4(1, 4, 0, 5),
	uint4(1, 3, 0, 6),
	uint4(1, 2, 0, 7),
	uint4(1, 1, 0, 8),
	uint4(8, 8, 9, 1),
	uint4(8, 7, 9, 2),
	uint4(8, 6, 9, 3),
	uint4(8, 5, 9, 4),
	uint4(8, 4, 9, 5),
	uint4(8, 3, 9, 6),
	uint4(8, 2, 9, 7),
	uint4(8, 1, 9, 8),
	uint4(8, 8, 0, 0),
	uint4(1, 8, 9, 0),
	uint4(8, 1, 0, 9),
	uint4(1, 1, 9, 9)
};
#endif


static const float kWeightEpsilon = 0.0001;

static const uint kCacheSize = THREAD_COUNT * THREAD_COUNT;

groupshared float4 RayCacheDirectionDepth[kCacheSize];

#if !defined(DDGI_UPDATE_DEPTH)
groupshared float3 RayCacheRadiance[kCacheSize];
#endif

[RootSignature(RS_DDGI_UPDATE)]
[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void main(uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint probeIndex = Gid.x;
    const uint3 probeCoord = DDGI_GetProbeCoord(probeIndex);
    const float3 probePos = DDGI_ProbeCoordToPosition(probeCoord);
    const float maxDistance = GetScene().DDGI.MaxDistance;
    
    // TODO: Handle probe offsets for depth updates
    
#ifdef DDGI_UPDATE_DEPTH
    float2 result = 0.0f;
    const uint2 pixelTopLeft = ProbeDepthPixel(probeCoord);
#else
    float3 result = 0.0f;
    const uint2 pixelTopLeft = ProbeColourPixel(probeCoord);
#endif
    float totalWeight = 0.0f;
    const uint2 pixelCurrent = pixelTopLeft + GTid.xy;
    const uint2 copyCoord = pixelTopLeft - 1;
    
    const float3 texelDirection = DecodeOct(((GTid.xy + 0.5) / (float2) THREAD_COUNT) * 2 - 1);
    uint remainingRays = push.NumRays;
    uint offset = 0;
    while (remainingRays > 0)
    {
        uint numRays = min(kCacheSize, remainingRays);
        
        if (groupIndex < numRays)
        {
            // RayCacheDirectionDepth[groupIndex] = ResourceHeap_GetTexture2D(GetScene().DDGI.RTDirectionDepthTexId)[probeIndex * DDGI_MAX_RAY_COUNT + groupIndex + offset];
            
#if !defined(DDGI_UPDATE_DEPTH)
            // RayCacheRadiance[groupIndex] = ResourceHeap_GetTexture2D(GetScene().DDGI.RTRadianceTexId)[probeIndex * DDGI_MAX_RAY_COUNT + groupIndex + offset].rgb;
#endif
        }
        
        GroupMemoryBarrierWithGroupSync();
        
        // Gather Ray data
        for (uint r = 0; r < numRays; ++r)
        {
            float4 rayDirectionDepth = RayCacheDirectionDepth[r];
            float3 rayDirection = rayDirectionDepth.xyz;

#if defined(DDGI_UPDATE_DEPTH)
            float rayProbeDistance = min(GetScene().DDGI.MaxDistance, rayDirectionDepth.w);
            if (rayProbeDistance == FLT_MAX)
            {
                rayProbeDistance = rayDirectionDepth.w;
            }
#else
            float3 rayProbeRadiance = RayCacheRadiance[r];
#endif
            float weight = saturate(dot(texelDirection, rayDirection));
            
#if defined(DDGI_UPDATE_DEPTH)
            weight = pow(weight, 64);
#endif
            if (weight > kWeightEpsilon)
            {
#if defined(DDGI_UPDATE_DEPTH)
                result += float2(rayProbeDistance, rayProbeDistance * rayProbeDistance) * weight;
#else
                result += rayProbeRadiance * weight;
#endif
                totalWeight += weight;

            }
        }
        
        GroupMemoryBarrierWithGroupSync();
        
        remainingRays -= numRays;
        offset += numRays;
    }
    
    if (totalWeight > kWeightEpsilon)
    {
        result /= totalWeight;
    }
    
    // Obtain previous frames results  
#if defined(DDGI_UPDATE_DEPTH)
    // TODO: Determine what textures these are
    const float2 prevResult = ResourceHeap_GetTexture2D(GetScene().DDGI.DepthTextureId)[pixelCurrent].xy;
#else
    const float3 prevResult = ResourceHeap_GetTexture2D(GetScene().DDGI.IrradianceTextureId)[pixelCurrent].xyz;
#endif
    
    if (push.FirstFrame > 0)
    {
        result = lerp(prevResult, result, push.BlendSpeed);
    }
    
    // Obtain previous frames results  
#if defined(DDGI_UPDATE_DEPTH)
    OutputVisibilityAtlas[pixelCurrent] = result;
    
    GroupMemoryBarrierWithGroupSync();
    
	// Copy depth borders:
    for (uint index = groupIndex; index < 68; index += THREAD_COUNT * THREAD_COUNT)
    {
        uint2 srcCoord = copyCoord + DDGI_DEPTH_BORDER_OFFSETS[index].xy;
        uint2 dstCoord = copyCoord + DDGI_DEPTH_BORDER_OFFSETS[index].zw;
        OutputVisibilityAtlas[dstCoord] = OutputVisibilityAtlas[srcCoord];
    }

    // TODO: Probe Offset
	
#else
    OutputIrradianceAtlas[pixelCurrent] = float4(result, 1.0f);
	DeviceMemoryBarrierWithGroupSync();

	// Copy color borders:
	for (uint index = groupIndex; index < 36; index += THREAD_COUNT * THREAD_COUNT)
	{
		uint2 srcCoord = copyCoord + DDGI_COLOR_BORDER_OFFSETS[index].xy;
		uint2 dstCoord = copyCoord + DDGI_COLOR_BORDER_OFFSETS[index].zw;
        OutputIrradianceAtlas[dstCoord] = OutputIrradianceAtlas[srcCoord];
    }
#endif
    
}

#endif 