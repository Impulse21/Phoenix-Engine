#define THREAD_COUNT 64 // Min Threads per wave on some platforms

#include "Globals.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"

RWByteAddressBuffer MeshletOutputBuffer : register(u0);

[numthreads(1, THREAD_COUNT, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex )
{
    uint meshInstanceIdx = Gid.x;
    MeshInstance meshInstance = LoadMeshInstance(meshInstanceIdx);
    for (uint i = 0; i < meshInstance.GeometryCount; i++)
    {
        uint geometryIdx = meshInstance.GeometryOffset + i;
        Geometry geometry = LoadGeometry(geometryIdx);
        for (uint j = 0; j < geometry.MeshletCount; j += THREAD_COUNT)
        {
            Meshlet meshlet;
            meshlet.MeshInstanceIdx = meshInstanceIdx;
            meshlet.GeometryIdx = geometryIdx;
            meshlet.PrimitiveOffset = j * MESHLET_TRIANGLE_COUNT;
            
            uint meshletIndex = meshInstance.MeshletOffset + geometry.MeshletOffset + j;
            MeshletOutputBuffer.Store<Meshlet>(meshletIndex * sizeof(Meshlet), meshlet);
        }

    }
    
}