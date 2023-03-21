#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"


bool IsFrustumVisible()
{
    //TODO:
    return true;
}

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint objectInstanceIdx = DTid.x;
    
    if (objectInstanceIdx < GetScene().InstanceCount)
    {
        ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIdx);
        Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
        // TODO Load Fustrum cull data
        // TODO Cull Frustrum
        // TODO Occlusion cull
    
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