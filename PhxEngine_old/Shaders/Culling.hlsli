#ifndef __CULLING_HLSLI__
#define __CULLING_HLSLI__



bool IsConeDegenerate(CullData c)
{
    return (c.NormalCone >> 24) == 0xff;
}

bool IntersectSphere(float3 centerA, float radiusA, float3 centerB, float radiusB)
{
    const float3 v = centerB - centerA;
    const float totalRadius = radiusA + radiusB;

    return dot(v, v) < totalRadius;
}

bool ConeCull(float4 normalCone, float3 centre, float3 viewPosition, float axis, float apexOffset, float scale)
{
    float3 apex = centre.xyz * apexOffset * scale;
    float3 view = normalize(viewPosition - apex);
    
    return dot(view, -axis) < normalCone.w;
}

// TODO: I don't really get this...It's from master programming.
uint GetCubeFaceMask(float3 cubeMapPos, float3 aabbMin, float3 aabbMax)
{
    float3 planeNormals[] = { float3(-1, 1, 0), float3(1, 1, 0), float3(1, 0, 1), float3(1, 0, -1), float3(0, 1, 1), float3(0, -1, 1) };
    float3 absPlaneNormals[] = { float3(1, 1, 0), float3(1, 1, 0), float3(1, 0, 1), float3(1, 0, 1), float3(0, 1, 1), float3(0, 1, 1) };

    float3 aabbCenter = (aabbMin + aabbMax) * 0.5f;

    float3 center = aabbMax - cubeMapPos;
    float3 extents = (aabbMax - aabbMin) * 0.5f;

    bool rp[6];
    bool rn[6];

    for (uint i = 0; i < 6; ++i)
    {
        float dist = dot(center, planeNormals[i]);
        float radius = dot(extents, absPlaneNormals[i]);

        rp[i] = dist > -radius;
        rn[i] = dist < radius;
    }

    uint fpx = (rn[0] && rp[1] && rp[2] && rp[3] && aabbMax.x > cubeMapPos.x) ? 1 : 0;
    uint fnx = (rp[0] && rn[1] && rn[2] && rn[3] && aabbMin.x < cubeMapPos.x) ? 1 : 0;
    uint fpy = (rp[0] && rp[1] && rp[4] && rn[5] && aabbMax.y > cubeMapPos.y) ? 1 : 0;
    uint fny = (rn[0] && rn[1] && rn[4] && rp[5] && aabbMin.y < cubeMapPos.y) ? 1 : 0;
    uint fpz = (rp[2] && rn[3] && rp[4] && rp[5] && aabbMax.z > cubeMapPos.z) ? 1 : 0;
    uint fnz = (rn[2] && rp[3] && rn[4] && rn[5] && aabbMin.z < cubeMapPos.z) ? 1 : 0;

    return fpx | (fnx << 1) | (fpy << 2) | (fny << 3) | (fpz << 4) | (fnz << 5);
}
#endif // __CULL_PASS_PASS_HLSL__
