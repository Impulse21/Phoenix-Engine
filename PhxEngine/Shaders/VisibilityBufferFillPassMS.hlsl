#pragma pack_matrix(row_major)

#include "Globals.hlsli"
#include "Defines.hlsli"
#include "GBuffer.hlsli"
#include "VertexBuffer.hlsli"

#define ROOT_SIG \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=22, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \

#define MAX_PRIMS_PER_MESHLET 128
#define MAX_VERTICES_PER_MESHLET 128
#define MAX_THREAD_COUNT 128

PUSH_CONSTANT(push, MeshletPushConstants);


struct VertexOut
{
    uint MeshletIndex : COLOR0;
    float3 Normal : NORMAL0;
    float3 PositionWS : POSITION0;
    float4 Position : SV_Position;
};

inline uint LoadPrimitiveIndices(uint index)
{
    StructuredBuffer<uint> primitiveIndices = ResourceDescriptorHeap[push.PrimitiveIndicesIdx];
    return primitiveIndices[index];
}

inline uint LoadUniqueVertexIB(uint index)
{
    return ResourceHeap_GetBuffer(push.UniqueVertexIBIdx).Load(index * 4);
}
inline Meshlet_NEW LoadMeshlet(uint index)
{
    StructuredBuffer<Meshlet_NEW> meshletBuffer = ResourceDescriptorHeap[push.MeshletsBufferIdx];
    return meshletBuffer[index];
}

uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet_NEW m, uint index)
{
    uint primitive = LoadPrimitiveIndices(m.PrimOffset + index);
    uint3 unpackedPrimitive = UnpackPrimitive(primitive);
    return unpackedPrimitive;
}

uint GetVertexIndex(Meshlet_NEW m, uint localIndex)
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
    out indices uint3 tris[MAX_PRIMS_PER_MESHLET],
    out vertices VertexOut verts[MAX_VERTICES_PER_MESHLET])
{
    Meshlet_NEW m = LoadMeshlet(push.SubsetOffset + gid);

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
