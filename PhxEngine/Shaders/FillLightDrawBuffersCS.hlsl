#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "Culling.hlsli"

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "IndirectDraw.hlsli"

PUSH_CONSTANT(push, FillLightDrawBuffers);


[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    RWStructuredBuffer<uint> perLightDrawCounts = ResourceDescriptorHeap[push.DrawBufferCounterIdx];
    if (DTid.x == 0)
    {
        // Reset Counts
        for (int i = 0; i < MAX_NUM_LIGHTS; i++)
        {
            perLightDrawCounts[i] = 0;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    uint lightIndex = DTid.x;
    if (lightIndex >= GetScene().LightCount)
    {
        return;
    }
    
    StructuredBuffer<uint> perLightInstanceCounts = ResourceDescriptorHeap[GetScene().PerLightMeshInstanceCounts];
    const uint numVisibleMeshlets = perLightInstanceCounts[lightIndex];
    
    RWStructuredBuffer<uint> drawCounterBuffer = ResourceDescriptorHeap[push.DrawBufferCounterIdx];
    
    if (numVisibleMeshlets > 0)
    {
        if (push.UseMeshlets)
        {
            MeshletDrawCommand meshletDrawCommand;
            meshletDrawCommand.DrawId = lightIndex;
            meshletDrawCommand.Indirect.GroupCountX = ROUNDUP(numVisibleMeshlets, AS_GROUP_SIZE);
            meshletDrawCommand.Indirect.GroupCountY = 1;
            meshletDrawCommand.Indirect.GroupCountZ = 1;
            
            AppendStructuredBuffer<MeshletDrawCommand> drawMeshletIndirectBuffer = ResourceDescriptorHeap[push.DrawBufferIdx];
            drawMeshletIndirectBuffer.Append(meshletDrawCommand);
        }
        else
        {
            StructuredBuffer<uint2> perLightMeshInstances = ResourceDescriptorHeap[GetScene().PerLightMeshInstances];
            RWStructuredBuffer<MeshDrawCommand> drawMeshIndirectBuffer = ResourceDescriptorHeap[push.DrawBufferIdx];
            InterlockedAdd(drawCounterBuffer[lightIndex], numVisibleMeshlets);
            Light l = LoadLight(lightIndex);
            const int drawBufferOffset = GetScene().InstanceCount * lightIndex;
            for (int i = 0; i < numVisibleMeshlets; i++)
            {
                uint objectInstanceIdx = perLightMeshInstances[i].x;
                ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIdx);
                Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
                                
                uint numInstances = 1;
                switch (l.GetType())
                {
                    case LIGHT_TYPE_DIRECTIONAL:
                        numInstances = 3; // TODO: MOVE THIS TO A DEFINE
                        break;
                    case LIGHT_TYPE_OMNI:
                        numInstances = 6; // TODO: MOVE THIS TO A DEFINE
                        break;
                    
                    case LIGHT_TYPE_SPOT:
                    default:
                        numInstances = 1;
                        break;
                }
                drawMeshIndirectBuffer[drawBufferOffset + i] = CreateMeshCommand(geometryData, objectInstanceIdx, numInstances);
            }
        }
    }
}