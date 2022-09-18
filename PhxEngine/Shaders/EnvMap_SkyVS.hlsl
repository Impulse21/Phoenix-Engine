
#include "Icosphere.hlsli"
#include "Globals.hlsli"
struct PSInput
{
    float3 NormalWS     : NORMAL;
	float4 Position : SV_POSITION;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
PSInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    PSInput output;

    // TODO: Translate to clip space.
    output.Position = sIcosphere[vertexID];
    output.NormalWS = sIcosphere[vertexID].xyz;
    return output;
}
