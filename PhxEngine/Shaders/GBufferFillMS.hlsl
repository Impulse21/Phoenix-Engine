#pragma pack_matrix(row_major)

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "Globals_NEW.hlsli"
#include "GBuffer_New.hlsli"
#include "VertexBuffer.hlsli"

struct Payload
{
    uint MeshletIndices[AS_GROUP_SIZE];
};

struct VertexOut
{
    uint MeshletIndex : COLOR0;
    float3 Normal : NORMAL0;
    float3 PositionWS : POSITION0;
    float4 Position : SV_Position;
};

PUSH_CONSTANT(push, GeometryPushConstant);

inline uint LoadPrimitiveIndices(uint index)
{
    StructuredBuffer<uint> primitiveIndices = ResourceDescriptorHeap[GetScene().MeshletPrimitiveIdx];
    return primitiveIndices[index];
}

inline uint LoadUniqueVertexIB(uint index)
{
    return ResourceHeap_GetBuffer(push.UniqueVertexIBIdx).Load(index * 4);
}
inline Meshlet LoadMeshlet(uint index)
{
    StructuredBuffer<Meshlet> meshletBuffer = ResourceDescriptorHeap[push.MeshletsBufferIdx];
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

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    Geometry geometry = LoadGeometry(push.GeometryIdx);

    VertexData vertexData = RetrieveVertexData(vertexIndex, geometry);
    matrix worldMatrix = push.WorldMatrix;
    
    VertexOut vout;
    vout.PositionWS = mul(vertexData.Position, worldMatrix).xyz;
    vout.Position = mul(float4(vout.PositionWS, 1), GetCamera().ViewProjection);
    vout.Normal = mul(float4(vertexData.Normal, 0), worldMatrix).xyz;
    vout.MeshletIndex = meshletIndex;

    return vout;
}

[RootSignature(ROOT_SIG)]
[numthreads(MAX_THREAD_COUNT, 1, 1)]
[outputtopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out indices uint3 tris[MAX_PRIMS],
    out vertices VertexOut verts[MAX_VERTS])
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    uint meshletIndex = payload.MeshletIndices[gid];
    
      // Catch any out-of-range indices (in case too many MS threadgroups were dispatched from AS)
    if (meshletIndex >= geometryData.MeshletCount)
    {
        return;
    }
    
    Meshlet m = LoadMeshlet(geometryData.MeshletOffset + gid);

    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if (gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }

    if (gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);
        verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    }
}
