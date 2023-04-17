#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "Culling.hlsli"

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"

PUSH_CONSTANT(push, FillPerLightListConstants);

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    RWStructuredBuffer<uint> perLightInstanceCounts = ResourceDescriptorHeap[push.PerLightMeshCountsUavIdx];
    RWStructuredBuffer<uint2> PerLightMeshletInstances = ResourceDescriptorHeap[push.PerLightMeshInstancesUavIdx];
    if (DTid.x == 0)
    {
        // Reset Counts
        for (int i = 0; i < MAX_NUM_LIGHTS; i++)
        {
            perLightInstanceCounts[i] = 0;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    uint lightIndex = DTid.x % GetScene().LightCount;
    if (lightIndex >= GetScene().LightCount)
    {
        return;
    }
    const Light light = LoadLight(lightIndex);
    
    // Check if light even intersects the camera at all?
    
    uint objectInstanceIdx = DTid.x / GetScene().LightCount;
    if (objectInstanceIdx >= GetScene().InstanceCount)
    {
        return;
    }
    ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIdx);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    float4 geometryBoundingSphere = LoadGeometryBounds(objectInstance.GeometryIndex);
    float4 boundCentreWS = mul(float4(geometryBoundingSphere.xyz, 1.0f), objectInstance.WorldMatrix);
    float radius = geometryBoundingSphere.w * 1.1;
    
    // TODO: Expand this check against frustums.
    
    const bool intersectsLight = (light.GetType() == LIGHT_TYPE_DIRECTIONAL) || IntersectSphere(boundCentreWS.xyz, radius, light.Position, light.GetRange());
    
    if (!intersectsLight)
    {
        return;
    }
    
    uint perLightOffset = 0;
    if (push.UseMeshlets)
    {
        InterlockedAdd(perLightInstanceCounts[lightIndex], geometryData.MeshletCount, perLightOffset);
    
         // Add the meshlet instances
        for (uint m = 0; m < geometryData.MeshletCount; m++)
        {
            const uint meshletIndex = geometryData.MeshletOffset + m;
            PerLightMeshletInstances[lightIndex * MAX_MESHLETS_PER_LIGHT + perLightOffset + m] = uint2(objectInstanceIdx, meshletIndex);
        }
    }
    else
    {
        InterlockedAdd(perLightInstanceCounts[lightIndex], 1, perLightOffset);
        PerLightMeshletInstances[lightIndex * MAX_MESHLETS_PER_LIGHT + perLightOffset] = uint2(objectInstanceIdx, -1);
    }
}