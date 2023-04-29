#ifndef __MESHLET_COMMON_HLSLI__
#define __MESHLET_COMMON_HLSLI__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "ResourceHeapTables.hlsli"

struct MeshletPayload
{
    uint MeshletIndices[AS_GROUP_SIZE];
};

struct MeshletShadowPayload
{
    uint MeshletIndices[6][AS_GROUP_SIZE];
};

inline uint LoadPrimitiveIndices(uint index)
{
    StructuredBuffer<uint> primitiveIndices = ResourceDescriptorHeap[GetScene().MeshletPrimitiveIdx];
    return primitiveIndices[index];
}

inline uint LoadUniqueVertexIB(uint index)
{
    return ResourceHeap_GetBuffer(GetScene().UniqueVertexIBIdx).Load(index * 4);
}
inline Meshlet LoadMeshlet(uint index)
{
    StructuredBuffer<Meshlet> meshletBuffer = ResourceDescriptorHeap[GetScene().MeshletBufferIdx];
    return meshletBuffer[index];
}

uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet m, uint index)
{
    uint primitive = LoadPrimitiveIndices(m.PrimOffset + index);
    uint3 unpackedPrimitive = UnpackPrimitive(primitive);
    return unpackedPrimitive;
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;
    
    return LoadUniqueVertexIB(localIndex);
}

#endif