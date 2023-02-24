#pragma pack_matrix(row_major)

#include "VertexBuffer.hlsli"
#include "VisibilityFillPass_Resources.hlsli"

[RootSignature(VisibilityBufferFillRS)]
PSInputVisBuffer main(
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