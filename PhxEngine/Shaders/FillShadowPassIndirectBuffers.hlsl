#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "Culling.hlsli"

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "IndirectDraw.hlsli"

PUSH_CONSTANT(push, LightCullPushConstants);


[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    /*
    uint lightIndex = DTid.x;
    if (lightIndex >= GetScene().LightCount)
    {
        return;
    }
    
    StructuredBuffer<uint> perLightInstances = ResourceDescriptorHeap[GetScene().PerLightMeshInstances];
    const uint numVisibleMeshlets = perLightInstances[lightIndex];
    
    if (numVisibleMeshlets > 0)
    {
        AppendStructuredBuffer<MeshDrawCommand> drawMeshIndirectBuffer = ResourceDescriptorHeap[GetScene().IndirectShadowPassMeshBufferIdx];
        AppendStructuredBuffer<MeshletDrawCommand> drawMeshletIndirectBuffer = ResourceDescriptorHeap[GetScene().IndirectShadowPassMeshletBufferIdx];
        
        const uint packedLightIndex = (lightIndex & 0xFFFF) << 16;
        const Light light = LoadLight(lightIndex);
        const uint lightType = light.GetType();
        switch (lightType)
        {
            case LIGHT_TYPE_OMNI:
                for (int i = 0; i < 6; i++)
                {
                    MeshDrawCommand meshDrawCommand = CreateMeshCommand(geometryData, packedLightIndex | i);
                    MeshletDrawCommand meshletDrawCommand = CreateMeshletCommand(geometryData, packedLightIndex | i);
                }
                break;
            case LIGHT_TYPE_DIRECTIONAL:
            case LIGHT_TYPE_SPOT:
                MeshDrawCommand meshDrawCommand = CreateMeshCommand(geometryData, packedLightIndex | 0);
                MeshletDrawCommand meshletDrawCommand = CreateMeshletCommand(geometryData, packedLightIndex | 0);
                break;
            default:
                break;
        }
    }
    */
}