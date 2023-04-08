#ifndef __INDIRECT_DRAW_HLSLI__
#define __INDIRECT_DRAW_HLSLI__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

MeshDrawCommand CreateMeshCommand(Geometry geometryData, uint drawId, uint miscData)
{
    MeshDrawCommand meshDrawCommand;
    meshDrawCommand.DrawId = drawId;
    //meshDrawCommand.MiscData = miscData;
    meshDrawCommand.Indirect.IndexCount = geometryData.NumIndices;
    meshDrawCommand.Indirect.InstanceCount = 1;
    meshDrawCommand.Indirect.StartIndex = geometryData.IndexOffset;
    meshDrawCommand.Indirect.VertexOffset = 0;
    meshDrawCommand.Indirect.StartInstance = 0;
    
    return meshDrawCommand;
}

MeshletDrawCommand CreateMeshletCommand(Geometry geometryData, uint drawId, uint miscData)
{
    MeshletDrawCommand meshletDrawCommand;
    meshletDrawCommand.DrawId = drawId;
    //meshletDrawCommand.MiscData = miscData;
    meshletDrawCommand.Indirect.GroupCountX = ROUNDUP(geometryData.MeshletCount, AS_GROUP_SIZE);
    meshletDrawCommand.Indirect.GroupCountY = 1;
    meshletDrawCommand.Indirect.GroupCountZ = 1;
    
    return meshletDrawCommand;
    
}
MeshDrawCommand CreateMeshCommand(Geometry geometryData, uint drawId)
{
    return CreateMeshCommand(geometryData, drawId, 0);
}

MeshletDrawCommand CreateMeshletCommand(Geometry geometryData, uint drawId)
{
    return CreateMeshletCommand(geometryData, drawId, 0);
    
}
#endif