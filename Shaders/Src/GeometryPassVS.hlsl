
#include "Globals.hlsli"

PUSH_CONSTANT(push, GeometryPassPushConstants);

inline Geometry GetGeometry()
{
    return LoadGeometry(push.GeometryIndex);
}

inline MaterialData GetMaterial(uint materialID)
{
    return LoadMaterial(materialID);
}

struct VsOutput
{
    float3 NormalWS     : NORMAL;
    float4 Colour       : COLOUR;
    float2 TexCoord : TEXCOORD;
    // float3 ShadowTexCoord : TEXCOORD1;
    float3 PositionWS   : Position;
    float4 TangentWS    : TANGENT;
    uint MaterialID : MATERIAL;
    float4 Position     : SV_POSITION;
};

VsOutput main(
    in uint vertexID : SV_VertexID)
{
    VsOutput output;

    Geometry geometry = LoadGeometry(push.GeometryIndex);
    ByteAddressBuffer vertexBuffer = ResourceHeap_Buffer[geometry.VertexBufferIndex];

    uint index = vertexID;
    
    matrix worldMatrix = push.WorldTransform;
    float4 position = float4(asfloat(vertexBuffer.Load3(geometry.PositionOffset + index * 12)), 1.0f);
    
    output.PositionWS = mul(position, worldMatrix).xyz;
    output.Position = mul(float4(output.PositionWS, 1.0f), GetCamera().ViewProjection);

    output.NormalWS = geometry.NormalOffset == ~0u ? 0 : asfloat(vertexBuffer.Load3(geometry.NormalOffset + index * 12));
    output.TexCoord = geometry.TexCoordOffset == ~0u ? 0 : asfloat(vertexBuffer.Load2(geometry.TexCoordOffset + index * 8));
    output.Colour = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    output.TangentWS = geometry.TangentOffset == ~0u ? 0 : asfloat(vertexBuffer.Load4(geometry.TangentOffset + index * 16));
    output.TangentWS = float4(mul(output.TangentWS.xyz, (float3x3) worldMatrix), output.TangentWS.w);
    
    output.MaterialID = geometry.MaterialIndex;

    return output;
}