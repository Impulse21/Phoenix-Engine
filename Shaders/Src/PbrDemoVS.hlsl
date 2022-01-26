#include "Defines.hlsli"

struct DrawPushConstant
{
    uint InstanceBufferIndex;
    uint GeometryBufferIndex;
};

struct SceneInfo
{
    matrix ViewProjection;

	// -- 16 byte boundary ----
    
    matrix ShadowViewProjection;

	// -- 16 byte boundary ----
    
    float3 CameraPosition;
    uint _padding;

	// -- 16 byte boundary ----
    
    float3 SunDirection;
    uint _padding1;

	// -- 16 byte boundary ----
    
    float3 SunColour;
    uint IrradianceMapTexIndex;

	// -- 16 byte boundary ----
    
    uint PreFilteredEnvMapTexIndex;
    uint BrdfLUTTexIndex;
    
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
ConstantBuffer<SceneInfo> SceneInfoCB : register(b1);

StructuredBuffer<MeshInstanceData> MeshInstanceSB : register(t0);
StructuredBuffer<GeometryData> GeometrySB : register(t1);

ByteAddressBuffer BufferTable[] : register(t0, BufferSpace);

struct VsOutput
{
    float3 NormalWS     : NORMAL;
    float4 Colour       : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 ShadowTexCoord : TEXCOORD1;
    float3 PositionWS   : Position;
    float4 TangentWS    : TANGENT;
    uint MaterialID : MATERIAL;
    float4 Position     : SV_POSITION;
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
    
    output.PositionWS = mul(modelPosition, worldMatrix).xyz;
    output.Position = mul(float4(output.PositionWS, 1.0f), SceneInfoCB.ViewProjection);
    output.ShadowTexCoord = mul(float4(output.PositionWS, 1.0f), SceneInfoCB.ShadowViewProjection).xyz;
    
    
    output.NormalWS = geometry.NormalOffset == ~0u ? 0 : asfloat(vertexBuffer.Load3(geometry.NormalOffset + index * 12));
    output.TexCoord = geometry.TexCoordOffset == ~0u ? 0 : asfloat(vertexBuffer.Load2(geometry.TexCoordOffset + index * 8));
    output.Colour = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    output.TangentWS = geometry.TangentOffset == ~0u ? 0 : asfloat(vertexBuffer.Load4(geometry.TangentOffset + index * 16));
    output.TangentWS = float4(mul(output.TangentWS.xyz, (float3x3) worldMatrix), output.TangentWS.w);
    
    output.MaterialID = geometry.MaterialIndex;
    return output;
}