#ifndef _STANDARD_OBJECT_PASS_HLSL__
#define _STANDARD_OBJECT_PASS_HLSL__


struct PSInput
{
    float3 NormalWS : NORMAL;
    float4 Colour : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 ShadowTexCoord : TEXCOORD1;
    float3 PositionWS : Position;
    float4 TangentWS : TANGENT;
    uint MaterialID : MATERIAL;
    float4 Position : SV_POSITION;
};


#ifdef COMPILE_VS
[RootSignature(VisibilityBufferFillRS)]
PSInput main(
	uint vertexID : SV_VertexID,
	uint instanceID : SV_InstanceID)
{
    DrawPacket packet = LoadDrawPacket(push.DrawPacketIndex);
    
    Geometry geometry = LoadGeometry(packet.GeometryIdx);
    float4 position = RetrieveVertexPosition(vertexID, geometry);
    
    MeshInstance meshInstance = LoadMeshInstance(packet.InstanceIdx);
    
    PSInputVisBuffer output;
    output.Position = mul(position, meshInstance.WorldMatrix);
    output.Position = mul(output.Position, GetCamera().ViewProjection);
    output.DrawId = instanceID;
    return output;
}
#else // COMPILE_VS
#endif