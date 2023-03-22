#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint objectInstanceIdx = DTid.x;
    
    if (objectInstanceIdx < GetScene().InstanceCount)
    {
        ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIdx);
        Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
        float4 geometryBoundingSphere = LoadGeometryBounds(objectInstance.GeometryIndex);
        
        // Transform into view projection space
        float4 boundCentreWS = mul(float4(geometryBoundingSphere.xyz, 1.0f), objectInstance.WorldMatrix);
        float4 boundCentreVS = mul(boundCentreWS, GetCamera().View);
        float radius = geometryBoundingSphere.w * 1.1;
        
        bool isVisibleFrustum = true;
        for (uint i = 0; i < 6; i++)
        {
            isVisibleFrustum = isVisibleFrustum && (dot(GetCamera().Planes[i], boundCentreVS) > -radius);
        }
        
        bool isVisibileOcclusion = true;
        
        if (isVisibleFrustum)
        {
        }
        
        if (isVisibleFrustum && isVisibileOcclusion)
        {
            MeshDrawCommand command;
            command.DrawId = objectInstanceIdx;
            command.Indirect.IndexCount = geometryData.NumIndices;
            command.Indirect.InstanceCount = 1;
            command.Indirect.StartIndex = geometryData.IndexOffset;
            command.Indirect.VertexOffset = 0;
            command.Indirect.StartInstance = 0;
            command.IndirectMS.GroupCountX = ROUNDUP(geometryData.MeshletCount, AS_GROUP_SIZE);
            command.IndirectMS.GroupCountY = 1;
            command.IndirectMS.GroupCountZ = 1;
    
            AppendStructuredBuffer<MeshDrawCommand> drawMeshIndirectBuffer = ResourceDescriptorHeap[GetScene().IndirectEarlyBufferIdx];
            drawMeshIndirectBuffer.Append(command);
        }
    }
}