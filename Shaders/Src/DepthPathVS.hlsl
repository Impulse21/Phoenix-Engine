#include "Defines.hlsli"

struct DrawPushConstant
{
    uint InstanceBufferIndex;
    uint GeometryBufferIndex;
    matrix ViewProjMatrix;
};

struct GeometryData
{
    uint NumIndices;
    uint NumVertices;
    uint IndexBufferIndex;
    uint IndexOffset;

	// -- 16 byte boundary ----

    int VertexBufferIndex;
    uint PositionOffset;
    uint TexCoordOffset;
    uint NormalOffset;

	// -- 16 byte boundary ----

    uint TangentOffset;
    uint MaterialIndex;
    uint _padding;
    uint _padding1;

	// -- 16 byte boundary ----
};

struct MeshInstanceData
{
    uint FirstGeometryIndex;
    uint NumGeometries;
    uint _padding;
    uint _padding1;

	// -- 16 byte boundary ----

    matrix Transform;

	// -- 16 byte boundary ----
};

ConstantBuffer<DrawPushConstant> DrawPushConstantCB : register(b0);

StructuredBuffer<MeshInstanceData> MeshInstanceSB : register(t0);
StructuredBuffer<GeometryData> GeometrySB : register(t1);

ByteAddressBuffer BufferTable[] : register(t0, BufferSpace);

struct VsOutput
{
    float4 Position : SV_POSITION;
};

VsOutput main(
    in uint vertexID : SV_VertexID)
{
    VsOutput output;
    
    MeshInstanceData instance = MeshInstanceSB[DrawPushConstantCB.InstanceBufferIndex];
    GeometryData geometry = GeometrySB[instance.FirstGeometryIndex + DrawPushConstantCB.GeometryBufferIndex];
    
    ByteAddressBuffer indexBuffer = BufferTable[geometry.IndexBufferIndex];
    ByteAddressBuffer vertexBuffer = BufferTable[geometry.VertexBufferIndex];
    
    // uint index = indexBuffer.Load(geometry.IndexOffset + vertexID * 4);
    uint index = vertexID;
    
    matrix worldMatrix = instance.Transform;
    float4 modelPosition = float4(asfloat(vertexBuffer.Load3(geometry.PositionOffset + index * 12)), 1.0f);
    
    output.Position = mul(modelPosition, worldMatrix);
    output.Position = mul(output.Position, DrawPushConstantCB.ViewProjMatrix);

    return output;
}