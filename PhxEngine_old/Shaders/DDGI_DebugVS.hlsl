
#include "Globals_New.hlsli"
#include "UVSphere.hlsli"

struct VSOutput
{
    float3 Normal : NORMAL;
    uint ProbeIndex : PROBEINDEX;
    float4 Position : SV_Position;
};


[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
VSOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSOutput output;
    output.Position = sUvSphere[vertexID];
    output.Normal = output.Position.xyz;
    output.Position.xyz *= GetScene().DDGI.MaxDistance * 0.05;
    output.ProbeIndex = instanceID;

    const float3 probeCoord = DDGI_GetProbeCoord(output.ProbeIndex);
    const float3 probePosition = DDGI_ProbeCoordToPosition(probeCoord);

    output.Position.xyz += probePosition;
    output.Position = mul(GetCamera().ViewProjection, output.Position);
    return output;
}