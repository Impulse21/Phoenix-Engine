#pragma pack_matrix(row_major)


#include "DDGI_Common.hlsli"
#include "VertexFetch.hlsli"
#include "BRDF.hlsli"
#include "Lighting_New.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#define RS_DDGI_RT \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=20, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE | DATA_STATIC_WHILE_SET_AT_EXECUTE) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

RWByteAddressBuffer OutputOffsetBuffer : register(u0);

PUSH_CONSTANT(push, DDGIPushConstants);

[RootSignature(RS_DDGI_RT)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint probeIndex = Gid.x;
    const uint totalProbes = GetScene().DDGI.ProbeCount;
    // Early out if index is not valid
    if (probeIndex >= totalProbes)
    {
        return;
    }
    
    int closestBackFaceIndex = -1;
    float closestBackFaceDistance = 100000000.f;

    int closestFrontFaceIndex = -1;
    float closest_frontface_distance = 100000000.f;

    int farthestFrontfaceIndex = -1;
    float farthestFrontfaceDistance = 0;

    int backfacesCount = 0;
    // For each ray cache front/backfaces index and distances.
    for (int rayIndex = 0; rayIndex < push.NumRays; ++rayIndex)
    {
        float2 samplePosition = float2(rayIndex, probeIndex);
        float rayDistance = ResourceHeap_GetTexture2D(GetScene().DDGI.RTRadianceTexId)[samplePosition].a;
        
        // Negative distance is stored for backface hits in the Ray-Tracing shader.
        if (rayDistance <= 0.0f)
        {
            ++backfacesCount;
            // Distance is a positive value, thus negate ray_distance as it is negative already if
            // we are inside this branch.
            if ((-rayDistance) < closestBackFaceDistance)
            {
                closestBackFaceDistance = rayDistance;
                closestBackFaceIndex = rayDistance;
            }
        }
        else
        {
            // Cache either closest or farther distance and indices for this ray.
            if (rayDistance < closestFrontFaceIndex)
            {
                closestFrontFaceIndex = rayDistance;
                closestFrontFaceIndex = rayIndex;
            }
            else if (rayDistance > farthestFrontfaceDistance)
            {
                farthestFrontfaceDistance = rayDistance;
                farthestFrontfaceIndex = rayIndex;
            }
        }
    }

    float3 fullOffset = 10000.f;
    float3 cellOffsetLimit = GetScene().DDGI.CellSize;

    float3 currentOffset = 0;
    // Read previous offset after the first frame.
    if (GetScene().DDGI.FrameIndex > 0)
    {
        currentOffset = OutputOffsetBuffer.Load<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset)).load();
    }

    // Check if a fourth of the rays was a backface, we can assume the probe is inside a geometry.
    const bool insideGeometry = (float(backfacesCount) / push.NumRays) > 0.25f;
    if (insideGeometry && (closestBackFaceIndex != -1))
    {
        const float3x3 randomRotation = (float3x3) push.RandRotation;
        // Calculate the backface direction.
        // NOTE: Distance is always positive
        const float3 closestBackfaceDirection = closestBackFaceDistance * normalize(mul(randomRotation, SphericalFibonacci(closestBackFaceIndex, push.NumRays)));
        
        // Find the maximum offset inside the cell.
        const float3 positiveOffset = (currentOffset + cellOffsetLimit) / closestBackfaceDirection;
        const float3 negativeOffset = (currentOffset - cellOffsetLimit) / closestBackfaceDirection;
        const float3 maximumOffset = float3(max(currentOffset.x, negativeOffset.x), max(positiveOffset.y, negativeOffset.y), max(positiveOffset.z, negativeOffset.z));

        // Get the smallest of the offsets to scale the direction
        const float directionScaleFactor = min(min(maximumOffset.x, maximumOffset.y), maximumOffset.z) - 0.001f;

        // Move the offset in the opposite direction of the backface one.
        fullOffset = currentOffset - closestBackfaceDirection * directionScaleFactor;
    }
    else if (closestFrontFaceIndex < 0.05f)
    {
        // In this case we have a very small hit distance.

        // Ensure that we never move through the farthest frontface
        // Move minimum distance to ensure not moving on a future iteration.
        
        const float3x3 randomRotation = (float3x3) push.RandRotation;
        const float3 farthestDirection = min(0.2f, farthestFrontfaceDistance) * normalize(mul(randomRotation, SphericalFibonacci(farthestFrontfaceIndex, push.NumRays)));
        const float3 closestDirection = normalize(mul(randomRotation, SphericalFibonacci(closestFrontFaceIndex, push.NumRays)));
        // The farthest frontface may also be the closest if the probe can only 
        // see one surface. If this is the case, don't move the probe.
        if (dot(farthestDirection, closestDirection) < 0.5f)
        {
            fullOffset = currentOffset + farthestDirection;
        }
    }

    // Move the probe only if the newly calculated offset is within the cell.
    if (all(abs(fullOffset) < GetScene().DDGI.CellSize))
    {
        currentOffset = fullOffset;
    }
    
    DDGIProbeOffset ofs;
    ofs.Store(currentOffset);
    ofs.Store(0);
    OutputOffsetBuffer.Store<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset), ofs);
}