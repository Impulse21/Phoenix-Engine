#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "Culling.hlsli"

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"

PUSH_CONSTANT(push, LightCullPushConstants);

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    RWStructuredBuffer<uint> perLightInstances = ResourceDescriptorHeap[push.PerLightMeshUavIdx];
    RWStructuredBuffer<uint2> lightMeshletInstances = ResourceDescriptorHeap[push.LightMeshletUavIdx];
    if (Gid.x == 0)
    {
        // Reset Counts
        for (int i = 0; i < MAX_NUM_LIGHTS; i++)
        {
            perLightInstances[i] = 0;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    uint lightIndex = Gid.x % GetScene().LightCount;
    if (lightIndex >= GetScene().LightCount)
    {
        return;
    }
    const Light light = LoadLight(lightIndex);
    
    uint objectInstanceIdx = Gid.x & GetScene().InstanceCount;
    if (objectInstanceIdx >= GetScene().InstanceCount)
    {
        return;
    }
    ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIdx);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    float4 geometryBoundingSphere = LoadGeometryBounds(objectInstance.GeometryIndex);
    float4 boundCentreWS = mul(float4(geometryBoundingSphere.xyz, 1.0f), objectInstance.WorldMatrix);
    float radius = geometryBoundingSphere.w * 1.1;
    
    const bool intersectsLight = (light.GetType() == LIGHT_TYPE_DIRECTIONAL) || IntersectSphere(boundCentreWS.xyz, radius, light.Position, light.GetRange());
    
    if (!intersectsLight)
    {
        return;
    }
    
    const uint perLightOffset = InterlockedAdd(perLightInstances[lightIndex], geometryData.MeshletCount);
    
    // Add the meshlet instances
    for (uint m = 0; m < geometryData.MeshletCount; m++)
    {
        const uint meshletIndex = geometryData.MeshletOffset + m;
        lightMeshletInstances[lightIndex * MAX_MESHLETS_PER_LIGHT + perLightOffset + m] = uint2(objectInstanceIdx, meshletIndex);
    }
}