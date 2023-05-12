#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "Defines.hlsli"
#include "VertexFetch.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"


PUSH_CONSTANT(push, DDGIPushConstants);

// spherical fibonacci: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_ray_trace.rgen
#define madfrac(A, B) ((A) * (B)-floor((A) * (B)))
static const float PHI = sqrt(5) * 0.5 + 0.5;
float3 SphericalFibonacci(float i, float n)
{
    float phi = 2.0 * PI * madfrac(i, PHI - 1);
    float cos_theta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sin_theta = sqrt(clamp(1.0 - cos_theta * cos_theta, 0.0f, 1.0f));
    return float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint probeIndex = Gid.x;
    const uint3 probeCoord = DDGI_GetProbeCoord(probeIndex);
    const uint3 probePos = DDGI_ProbeCoordToPosition(probeCoord);
    
    for (uint rayIndex = groupIndex; rayIndex < push.NumRays; rayIndex += THREADS_PER_WAVE)
    {
        RayDesc ray;
        ray.Origin = probePos;
        ray.TMin = 0; // Not tracing from a surface, so this can be zero since there is no concern with self hits
        ray.TMax = FLT_MAX;
        
        // Select a random location along the spherical Fibonacci and a random orientation matrix.
        ray.Direction = normalize(mul(push.RandRotation, SphericalFibonacci(rayIndex, push.NumRays)));

        RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_FORCE_OPAQUE> rayQuery;
        
        rayQuery.TraceRayInline(
			ResourceHeap_GetRTAccelStructure(GetScene().RT_TlasIndex),
			RAY_FLAG_NONE, // OR'd with flags above
			0xff, // Include all instances
			ray);
        
        while (rayQuery.Proceed());
        if (rayQuery.ComittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            // Set radiance for sky colour
            // Set this to zero
        }
        else
        {
            // We hit a surface
            
            // Calculate distance
            ray.Origin = rayQuery.WorldRayOrigin() + rayQuery.WorldRayDirection() * rayQuery.CommittedRayT();
            float hitDepth = rayQuery.CommittedRayT();
            const uint instanceIndex = rayQuery.CommittedInstanceID();
            
            // Load the instance data
            
            ObjectInstance objectInstance = LoadObjectInstnace(instanceIndex);
            Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
            
            // Get start Index of the primitive in the global index buffer
            const uint primitiveIndex = rayQuery.CommittedPrimitiveIndex();
            const uint startIndex = primitiveIndex * 3 + geometryData.IndexOffset;
            
            Buffer<uint> indexBuffer = ResourceDescriptorHeap[GetScene().GlobalIndexBufferIdx];
            uint i0 = indexBuffer[startIndex + 0];
            uint i1 = indexBuffer[startIndex + 1];
            uint i2 = indexBuffer[startIndex + 2];
            
            // Get Position Data
            VertexData v0 = FetchVertexData(i0, geometryData);
            VertexData v1 = FetchVertexData(i1, geometryData);
            VertexData v2 = FetchVertexData(i2, geometryData);
            
            // Now we can calculate the position using the baryCentric weights
            // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#rayquery-committedinstanceid
            const float2 barycentricWeights = rayQuery.CommittedTriangleBarycentrics();
            BarycentricInterpolation(v0.Position, v1.Position, v2.Position, barycentricWeights);
            
            const uint subsetIndex = rayQuery.CommittedGeometryIndex();


            // calculate the position and 
            // TODO: Construct surface.
        }

    }

}