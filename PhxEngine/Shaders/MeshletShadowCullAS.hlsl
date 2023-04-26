#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "MeshletCommon.hlsli"
#include "Culling.hlsli"

groupshared MeshletPayload s_payload;

PUSH_CONSTANT(push, GeometryPushConstant);

// DirectX Samples 
inline bool IsVisible(CullData cullData, Light light, float4x4 world, float scale, uint faceIndex)
{
    // TODO: Check if culling is enabled otherwise skip
    if (GetFrame().Flags & FRAME_FLAGS_DISABLE_CULL_MESHLET)
    {
        return true;
    }
    
    float3 centre = mul(cullData.BoundingSphere.xyz, light.Position);
    float radius = cullData.BoundingSphere.w * scale;
    
    // Do cone culling
    if (cullData.IsConeDegenerate())
    {
        return false; // Cone is degenerate - spread is wider than a hemisphere.
    }
    
    float4 normalCone = cullData.UnpackCone();
    
    // Transform the Access into world space.
    float3 axis = normalize(mul(float4(normalCone.xyz, 0), world)).xyz;
    
    if (ConeCull(normalCone, centre, light.Position, axis, cullData.ApexOffset, scale))
    {
        return false;
    }
    
    if (light.GetType() == LIGHT_TYPE_DIRECTIONAL)
    {
        return true;
    }
    
    // Check against light spehere
    if (!IntersectSphere(centre, radius, light.Position, light.GetRange()))
    {
        return false;
    }
    
    // Check against Frustra faces
    
    uint visibleFaces = GetCubeFaceMask(light.Position, centre - float3(radius, radius, radius), centre + float3(radius, radius, radius));
    
    bool accept = false;
    switch (faceIndex)
    {
        case 0:
            accept = (visibleFaces & 1) != 0;
            break;
        case 1:
            accept = (visibleFaces & 2) != 0;
            break;
        case 2:
            accept = (visibleFaces & 4) != 0;
            break;
        case 3:
            accept = (visibleFaces & 8) != 0;
            break;
        case 4:
            accept = (visibleFaces & 16) != 0;
            break;
        case 5:
            accept = (visibleFaces & 32) != 0;
            break;
    }
    
    return accept;
}

uint2 LoadPerLightMeshletInstance(uint index)
{
    StructuredBuffer<uint2> buffer = ResourceDescriptorHeap[GetScene().PerLightMeshInstances];
    return buffer[index];
}

[numthreads(AS_GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    bool isVisibile = false;
    
    const uint lightIndex = push.DrawId;
    // Load instance and meshlet
    const uint meshletIndex = DTid.x;
    const uint meshletIndexReadOffset = lightIndex * MAX_MESHLETS_PER_LIGHT;
    const uint2 lightMeshletInstance = LoadPerLightMeshletInstance(meshletIndexReadOffset + meshletIndex);
    const uint globalMeshletIndex = lightMeshletInstance.y;
    const uint objectInstanceIndex = lightMeshletInstance.x;
    
    // Need to load the global light and meshlet info 
    
    // Could better utilize the threading here if we actually dispatch per face
    const Light light = LoadLight(push.DrawId);
    const ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIndex);
   
    CullData cullData = LoadMeshletCullData(globalMeshletIndex);
    
    switch (light.GetType())
    {
        case LIGHT_TYPE_DIRECTIONAL:
        case
            
        
    }
    isVisibile = IsVisible(cullData, light, objectInstance.WorldMatrix, 1);
        
    if (isVisibile)
    {
        uint index = WavePrefixCountBits(isVisibile);
        s_payload.MeshletIndices[index] = DTid.x;
    }
    
    uint visibleCount = WaveActiveCountBits(isVisibile);
    DispatchMesh(visibleCount, 1, 1, s_payload);
}
