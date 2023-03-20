#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "MeshletCommon.hlsli"

groupshared MeshletPayload s_payload;

PUSH_CONSTANT(push, GeometryPushConstant);

bool IsConeDegenerate(CullData c)
{
    return (c.NormalCone >> 24) == 0xff;
}

// DirectX Samples 
inline bool IsVisible(CullData cullData, Camera camera, float4x4 world, float scale)
{
    // TODO: Check if culling is enabled otherwise skip
    if (GetFrame().Flags & FRAME_FLAGS_DISABLE_CULL_MESHLET)
    {
        return true;
    }
    float3 centre = mul(cullData.BoundingSphere.xyz, camera.GetPosition());
    float radius = cullData.BoundingSphere.w * scale;
    
    // Do a plane check
    for (int i = 0; i < 6; ++i)
    {
        if (dot(float4(centre, 1.0f), camera.Planes[i]) < -radius)
        {
            return false;
        }
    }
    
    // Do cone culling
    if (cullData.IsConeDegenerate())
    {
        return true; // Cone is degenerate - spread is wider than a hemisphere.
    }
    
    float4 normalCone = cullData.UnpackCone();
    
    // Transform the Access into world space.
    float3 axis = normalize(mul(float4(normalCone.xyz, 0), world)).xyz;
    
    float3 apex = centre.xyz * cullData.ApexOffset * scale;
    float3 view = normalize(camera.GetPosition() - apex);
    
    // The normal cone w-component stores -cos(angle + 90 deg)
    // This is the min dot product along the inverted axis from which all the meshlet's triangles are backface
    if (dot(view, -axis) > normalCone.w)
    {
        return false;
    }
    
    return true;
}

[numthreads(AS_GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    bool isVisibile = false;
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    if (DTid.x < geometryData.MeshletCount)
    {
        CullData cullData = LoadMeshletCullData(DTid.x + geometryData.MeshletOffset);
        isVisibile = IsVisible(cullData, GetCamera(), objectInstance.WorldMatrix, 1);
    }
    
    if (isVisibile)
    {
        uint index = WavePrefixCountBits(isVisibile);
        s_payload.MeshletIndices[index] = DTid.x;
    }
    
    uint visibleCount = WaveActiveCountBits(isVisibile);
    DispatchMesh(visibleCount, 1, 1, s_payload);
}
