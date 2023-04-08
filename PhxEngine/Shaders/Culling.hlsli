#ifndef __CULLING_HLSLI__
#define __CULLING_HLSLI__



bool IntersectSphere(float3 centerA, float radiusA, float3 centerB, float radiusB)
{
    const float3 v = centerB - centerA;
    const float totalRadius = radiusA + radiusB;

    return dot(v, v) < totalRadius;
}

#endif // __CULL_PASS_PASS_HLSL__
