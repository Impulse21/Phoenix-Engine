#ifndef __DDGI_UPDATE_HLSLI__
#define __DDGI_UPDATE_HLSLI__

#include "Globals_NEW.hlsli"
#include "DDGI_Common.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "Defines.hlsli"

#define THREAD_COUNT 8

#define RS_DDGI_UPDATE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=20, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 2, flags = DESCRIPTORS_VOLATILE | DATA_STATIC_WHILE_SET_AT_EXECUTE) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

PUSH_CONSTANT(push, DDGIPushConstants);
#ifdef DDGI_UPDATE_DEPTH
RWTexture2D<float2> OutputVisibilityAtlas : register(u0);
#else
RWTexture2D<float4> OutputIrradianceAtlas : register(u0);
#endif

static const int kReadTable[6] = { 5, 3, 1, -1, -3, -5 };
static const float kWeightEpsilon = 0.0001;

[RootSignature(RS_DDGI_UPDATE)]
[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    /* Data Drive these variables */
    const bool useBackFaceBlending = true;
    const bool showBorderVSInside = false;
    const bool usePerceptualEncoding = false;
    const bool showBorderType = false;
    const bool showBorderSourceCoordinates = false;
    /*END */
    
    uint2 coords = DTid.xy;
    
    // TODO: Handle probe offsets for depth updates
#ifdef DDGI_UPDATE_DEPTH
    const int probeTextureWidth = GetScene().DDGI.VisibilityTextureResolution.x;
    const int probeTextureHeight = GetScene().DDGI.VisibilityTextureResolution.y;
    const uint probeSideLength = DDGI_DEPTH_RESOLUTION;
#else
    const int probeTextureWidth = GetScene().DDGI.IrradianceTextureResolution.x;
    const int probeTextureHeight = GetScene().DDGI.IrradianceTextureResolution.y;
    const uint probeSideLength = DDGI_COLOUR_RESOLUTION;
#endif 
    // Early out for 1 pixel border around all image and outside bound pixels.
    if (coords.x >= probeTextureWidth || coords.y >= probeTextureHeight)
    {
        return;
    }
    
    const uint probeWithBorderSide = probeSideLength + 2;
    const uint probeLastPixel = probeSideLength + 1;
    int probeIndex = ProbeIndexFromPixels(coords.xy, int(probeWithBorderSide), probeTextureWidth);
    
    bool borderPixel = ((DTid.x % probeWithBorderSide) == 0) || ((DTid.x % probeWithBorderSide) == probeLastPixel);
    borderPixel = borderPixel || ((DTid.y % probeWithBorderSide) == 0) || ((DTid.y % probeWithBorderSide) == probeLastPixel);
	
	// Perform Calculations
	if (!borderPixel)
    {
        const uint maxBackFaces = uint(push.NumRays * 0.1f);
		
        float4 result = 0.0f;
        const float energyConservation = 0.95;
        uint backFaces = 0;
        const float3x3 randomRotation = (float3x3) push.RandRotation;
        for (int rayIndex = 0; rayIndex < push.NumRays; ++rayIndex)
        {
            const float2 samplePosition = float2(rayIndex, probeIndex);
            const float3 rayDirection = normalize(mul(randomRotation, SphericalFibonacci(rayIndex, push.NumRays)));
            const float3 texelDirection = DecodeOct(NormalizedOctCoord(coords.xy, probeSideLength));
            
            float weight = max(0.0, dot(texelDirection, rayDirection));
            
            float distance = ResourceHeap_GetTexture2D(GetScene().DDGI.RTRadianceTexId)[samplePosition].a;
            if (distance < 0.0f && useBackFaceBlending)
            {
                ++backFaces;
                // Only blend if the max number of backfaces threshold hasn't been exceeded
                if (backFaces >= maxBackFaces)
                {
                    return;
                }
                
                continue;
            }
         
#ifdef DDGI_UPDATE_DEPTH
            const float maxDistance = GetScene().DDGI.MaxDistance;
            // increase or decresse the filtered distance value's "sharpness"
            weight = pow(weight, 2.5f);
            if (weight >= kWeightEpsilon)
            {
                distance = min(abs(distance), maxDistance);
                float3 value = float3(distance, SQR(distance), 0.0f);
                // Store total weight in Alpha.
                result += float4(value * weight, weight);
            }
#else
            if (weight >= kWeightEpsilon)
            {
                float3 radiance = ResourceHeap_GetTexture2D(GetScene().DDGI.RTRadianceTexId)[samplePosition].rgb;
                radiance *= energyConservation;
                
                // storing the sum of the weights in alpha temporarily
                result += float4(radiance * weight, weight);
            }
#endif
        }
        
        if (result.w > kWeightEpsilon)
        {
            result.xyz /= result.w;
            result.w = 1.0f;
        }
        
    // Obtain previous frames results  
#if defined(DDGI_UPDATE_DEPTH)
        const float2 prevResult = ResourceHeap_GetTexture2D(GetScene().DDGI.VisibilityAtlasTextureIdPrev)[coords.xy].rg;
#else
        const float3 prevResult = ResourceHeap_GetTexture2D(GetScene().DDGI.IrradianceAtlasTextureIdPrev)[coords.xy].rgb;
#endif
    
        // Debug inside with color green
        if (showBorderVSInside)
        {
            result = float4(0, 1, 0, 1);
        }
           // Obtain previous frames results  
#if defined(DDGI_UPDATE_DEPTH)
        if (GetScene().DDGI.FrameIndex > 0)
        {
            result = float4(lerp(result.rg, prevResult, push.Hysteresis), result.b, result.w);
        }
        OutputVisibilityAtlas[coords.xy] = float4(result.rg, 0.0f, 1.0f);
#else
        if (usePerceptualEncoding)
        {
            const float3 exp = 1.0f / 5.0f;
            result.rbg = pow(result.rgb, exp);
        }
        
        if (GetScene().DDGI.FrameIndex > 0)
        {
            result = float4(lerp(result.rgb, prevResult, push.Hysteresis), result.w);
        }
        OutputIrradianceAtlas[coords.xy] = result;
#endif  
        
        return;
    }
    
	DeviceMemoryBarrierWithGroupSync();
	
    
    // Copy border pixel calculating source pixels.
    const uint probePixelX = DTid.x % probeWithBorderSide;
    const uint probePixelY = DTid.y % probeWithBorderSide;
    bool cornerPixel = (probePixelX == 0 || probePixelX == probeLastPixel) &&
                        (probePixelY == 0 || probePixelY == probeLastPixel);
    bool rowPixel = (probePixelX > 0 && probePixelX < probeLastPixel);

    int2 sourcePixelCoordinate = coords.xy;

    if (cornerPixel)
    {
        sourcePixelCoordinate.x += probePixelX == 0 ? probeSideLength : -probeSideLength;
        sourcePixelCoordinate.y += probePixelY == 0 ? probeSideLength : -probeSideLength;

        if (showBorderType)
        {
            sourcePixelCoordinate = int2(2, 2);
        }
    }
    else if (rowPixel)
    {
        sourcePixelCoordinate.x += kReadTable[probePixelX - 1];
        sourcePixelCoordinate.y += (probePixelY > 0) ? -1 : 1;

        if (showBorderType)
        {
            sourcePixelCoordinate = int2(3, 3);
        }
    }
    else
    {
        sourcePixelCoordinate.x += (probePixelX > 0) ? -1 : 1;
        sourcePixelCoordinate.y += kReadTable[probePixelY - 1];

        if (showBorderType)
        {
            sourcePixelCoordinate = int2(4, 4);
        }
    }

#if defined(DDGI_UPDATE_DEPTH)
    float4 copiedData = float4(OutputVisibilityAtlas[sourcePixelCoordinate], 0, 1);
#else
    float4 copiedData = OutputIrradianceAtlas[sourcePixelCoordinate];
#endif

    // Debug border source coordinates
    if (showBorderSourceCoordinates)
    {
        copiedData = float4(coords.xy, sourcePixelCoordinate);
    }

    // Debug border with color red
    if (showBorderVSInside)
    {
        copiedData = float4(1, 0, 0, 1);
    }
    
#if defined(DDGI_UPDATE_DEPTH)
    OutputVisibilityAtlas[coords] = copiedData;
#else
    OutputIrradianceAtlas[coords] = copiedData;
#endif
}

#endif 